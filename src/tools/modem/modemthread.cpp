/***************************************************************************************************
 * Copyright 2026-2026 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "modemthread.h"

#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QScopeGuard>
#include <QSerialPort>

#include <functional>

namespace {
using TrafficLogger = std::function<void(const QByteArray&)>;

constexpr char SOH = 0x01;
constexpr char STX = 0x02;
constexpr char EOT = 0x04;
constexpr char ACK = 0x06;
constexpr char NAK = 0x15;
constexpr char CAN = 0x18;
constexpr char CCH = 'C';
constexpr char CPM_EOF = 0x1A;
constexpr char ZPAD = '*';
constexpr char ZDLE = 0x18;
constexpr char ZDATA = 'A';

quint16 crc16XModem(const QByteArray& data)
{
    quint16 crc = 0;
    for (unsigned char b : data) {
        crc ^= static_cast<quint16>(b) << 8;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = static_cast<quint16>((crc << 1) ^ 0x1021);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

quint8 checksum8(const QByteArray& data)
{
    quint8 sum = 0;
    for (unsigned char b : data) {
        sum = static_cast<quint8>(sum + b);
    }

    return sum;
}

QString bytesToHexString(const QByteArray& bytes)
{
    if (bytes.isEmpty()) {
        return QStringLiteral("<empty>");
    }

    return QString::fromLatin1(bytes.toHex(' ').toUpper());
}

bool waitForWritable(QSerialPort& serial, int timeoutMs)
{
    if (serial.bytesToWrite() == 0) {
        return true;
    }

    return serial.waitForBytesWritten(timeoutMs);
}

bool writeAll(QSerialPort& serial,
              const QByteArray& bytes,
              int timeoutMs,
              const TrafficLogger& logger = TrafficLogger())
{
    if (serial.write(bytes) != bytes.size()) {
        return false;
    }

    if (logger) {
        logger(bytes);
    }

    return waitForWritable(serial, timeoutMs);
}

void disconnectDevice(QSerialPort& serial)
{
    if (!serial.isOpen()) {
        return;
    }

    serial.clear(QSerialPort::AllDirections);
    serial.close();
}

bool readExact(QSerialPort& serial,
               QByteArray& out,
               int bytes,
               int timeoutMs,
               const TrafficLogger& logger = TrafficLogger())
{
    out.clear();
    while (out.size() < bytes) {
        if (!serial.waitForReadyRead(timeoutMs)) {
            return false;
        }

        out.append(serial.read(bytes - out.size()));
    }

    if (logger) {
        logger(out);
    }

    return true;
}

bool readByte(QSerialPort& serial,
              char& byte,
              int timeoutMs,
              const TrafficLogger& logger = TrafficLogger())
{
    QByteArray data;
    if (!readExact(serial, data, 1, timeoutMs, logger)) {
        return false;
    }

    byte = data.at(0);
    return true;
}

QByteArray buildXyFrame(bool oneK, quint8 block, const QByteArray& payload, bool useCrc = true)
{
    const int payloadSize = oneK ? 1024 : 128;
    QByteArray frame;
    frame.reserve(3 + payloadSize + (useCrc ? 2 : 1));
    frame.append(oneK ? STX : SOH);
    frame.append(static_cast<char>(block));
    frame.append(static_cast<char>(0xFF - block));

    QByteArray body = payload.left(payloadSize);
    if (body.size() < payloadSize) {
        body.append(QByteArray(payloadSize - body.size(), CPM_EOF));
    }

    frame.append(body);
    if (useCrc) {
        const quint16 crc = crc16XModem(body);
        frame.append(static_cast<char>((crc >> 8) & 0xFF));
        frame.append(static_cast<char>(crc & 0xFF));
    } else {
        frame.append(static_cast<char>(checksum8(body)));
    }

    return frame;
}

bool waitControlReply(QSerialPort& serial,
                      char& reply,
                      int timeoutMs,
                      const TrafficLogger& logger = TrafficLogger())
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        const int remain = timeoutMs - static_cast<int>(timer.elapsed());
        char ch = 0;
        if (!readByte(serial, ch, qMin(500, qMax(1, remain)), logger)) {
            continue;
        }

        if (ch == ACK || ch == NAK || ch == CAN || ch == CCH) {
            reply = ch;
            return true;
        }
    }

    return false;
}

bool waitFirstFrameWithHandshake(QSerialPort& serial,
                                 char handshake,
                                 char& head,
                                 int totalTimeoutMs,
                                 int intervalMs,
                                 const TrafficLogger& txLogger = TrafficLogger(),
                                 const TrafficLogger& rxLogger = TrafficLogger())
{
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < totalTimeoutMs) {
        if (!writeAll(serial, QByteArray(1, handshake), 1000, txLogger)) {
            return false;
        }

        const int remain = totalTimeoutMs - static_cast<int>(timer.elapsed());
        const int waitMs = qMin(intervalMs, qMax(1, remain));
        if (!readByte(serial, head, waitMs, rxLogger)) {
            continue;
        }

        if (head == SOH || head == STX || head == EOT || head == CAN) {
            return true;
        }
    }

    return false;
}

bool validateXyFrameCrc(const QByteArray& payload, const QByteArray& crcBytes)
{
    if (crcBytes.size() != 2) {
        return false;
    }

    const quint16 expected = static_cast<quint16>((static_cast<quint8>(crcBytes.at(0)) << 8)
                                                  | static_cast<quint8>(crcBytes.at(1)));
    return crc16XModem(payload) == expected;
}

bool validateXyFrameChecksum(const QByteArray& payload, const QByteArray& sumByte)
{
    if (sumByte.size() != 1) {
        return false;
    }

    return static_cast<quint8>(sumByte.at(0)) == checksum8(payload);
}

QString modeName(ModemThread::WorkMode mode)
{
    return mode == ModemThread::WorkMode::Tx ? QStringLiteral("TX") : QStringLiteral("RX");
}

QString protocolName(ModemThread::Protocol protocol)
{
    switch (protocol) {
    case ModemThread::Protocol::xModem:
        return QStringLiteral("XMODEM");
    case ModemThread::Protocol::yModem:
        return QStringLiteral("YMODEM");
    case ModemThread::Protocol::zModem:
        return QStringLiteral("ZMODEM");
    default:
        return QStringLiteral("UNKNOWN");
    }
}
} // namespace

ModemThread::ModemThread(QObject* parent)
    : QThread(parent)
{}

ModemThread::~ModemThread()
{
    stopTransfer();
    wait();
}

void ModemThread::setTaskConfig(const TaskConfig& config)
{
    QMutexLocker locker(&m_configMutex);
    m_config = config;
}

void ModemThread::stopTransfer()
{
    requestInterruption();
}

void ModemThread::logTraffic(const QString& direction, const QByteArray& bytes)
{
    emit information(QStringLiteral("%1: %2").arg(direction, bytesToHexString(bytes)));
}

void ModemThread::run()
{
    const bool ok = runTransfer();
    if (isInterruptionRequested()) {
        emit transferFinished(false, tr("Transfer canceled."));
    } else if (ok) {
        emit transferFinished(true, tr("Transfer completed successfully."));
    } else {
        emit transferFinished(false, tr("Transfer aborted and device disconnected."));
    }
}

bool ModemThread::runTransfer()
{
    TaskConfig cfg;
    {
        QMutexLocker locker(&m_configMutex);
        cfg = m_config;
    }

    emit progressChanged(0);

    const QString pName = protocolName(cfg.protocol);
    const QString mName = modeName(cfg.workMode);
    const QString port = cfg.portName;
    emit information(QStringLiteral("%1 %2 @ %3").arg(pName, mName, port));

    if (cfg.workMode == WorkMode::Tx) {
        return runTx(cfg);
    } else {
        return runRx(cfg);
    }
}

bool ModemThread::runTx(const TaskConfig& cfg)
{
    switch (cfg.protocol) {
    case Protocol::xModem:
        return txXModem(cfg);
    case Protocol::yModem:
        return txYModem(cfg);
    case Protocol::zModem:
        return txZModem(cfg);
    default:
        emit errorOccurred(tr("Unsupported protocol."));
        return false;
    }
}

bool ModemThread::runRx(const TaskConfig& cfg)
{
    switch (cfg.protocol) {
    case Protocol::xModem:
        return rxXModem(cfg);
    case Protocol::yModem:
        return rxYModem(cfg);
    case Protocol::zModem:
        return rxZModem(cfg);
    default:
        emit errorOccurred(tr("Unsupported protocol."));
        return false;
    }
}

bool ModemThread::txXModem(const TaskConfig& cfg)
{
    const auto txLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("TX"), bytes);
    };
    const auto rxLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("RX"), bytes);
    };

    QFile file(cfg.filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(tr("Failed to open file: %1").arg(file.errorString()));
        return false;
    }

    QSerialPort serial;
    serial.setPortName(cfg.portName);
    serial.setBaudRate(cfg.baudRate);
    serial.setDataBits(static_cast<QSerialPort::DataBits>(cfg.dataBits));
    serial.setParity(static_cast<QSerialPort::Parity>(cfg.parity));
    serial.setStopBits(static_cast<QSerialPort::StopBits>(cfg.stopBits));
    serial.setFlowControl(static_cast<QSerialPort::FlowControl>(cfg.flowControl));
    if (!serial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("Failed to open serial port: %1").arg(serial.errorString()));
        return false;
    }

    const auto serialGuard = qScopeGuard([&serial]() { disconnectDevice(serial); });

    serial.clear(QSerialPort::AllDirections);

    char req = 0;
    bool syncOk = false;
    bool seenNak = false;
    for (int i = 0; i < 30 && !isInterruptionRequested(); ++i) {
        if (!readByte(serial, req, 1000, rxLogger)) {
            continue;
        }

        if (req == CCH) {
            syncOk = true;
            break;
        }

        if (req == NAK) {
            seenNak = true;
        }
    }

    if (!syncOk && seenNak) {
        req = NAK;
        syncOk = true;
    }

    if (!syncOk || isInterruptionRequested()) {
        emit errorOccurred(tr("XMODEM sync timeout."));
        return false;
    }

    const bool useCrc = (req == CCH);
    emit information(useCrc ? tr("XMODEM mode: CRC") : tr("XMODEM mode: Checksum"));

    const qint64 total = qMax<qint64>(1, file.size());
    quint8 block = 1;
    qint64 sent = 0;
    while (!isInterruptionRequested()) {
        const QByteArray data = file.read(128);
        if (data.isEmpty()) {
            break;
        }

        emit information(tr("Send block #%1 (%2 bytes)").arg(block).arg(data.size()));
        const QByteArray frame = buildXyFrame(false, block, data, useCrc);
        bool acked = false;
        for (int retry = 0; retry < 10 && !isInterruptionRequested(); ++retry) {
            if (!writeAll(serial, frame, 2000, txLogger)) {
                continue;
            }

            char reply = 0;
            if (!waitControlReply(serial, reply, 5000, rxLogger)) {
                continue;
            }

            if (reply == ACK) {
                acked = true;
                break;
            }

            if (reply == CAN) {
                emit errorOccurred(tr("Transfer canceled by peer."));
                return false;
            }

            // NAK or repeated 'C' means receiver requests retransmission.
        }

        if (!acked || isInterruptionRequested()) {
            emit errorOccurred(tr("XMODEM frame not acknowledged."));
            return false;
        }

        sent += data.size();
        emit progressChanged(static_cast<int>((sent * 100) / total));
        ++block;
    }

    for (int i = 0; i < 10 && !isInterruptionRequested(); ++i) {
        if (!writeAll(serial, QByteArray(1, EOT), 1000, txLogger)) {
            continue;
        }

        char reply = 0;
        if (readByte(serial, reply, 5000, rxLogger) && reply == ACK) {
            emit progressChanged(100);
            return true;
        }
    }

    emit errorOccurred(tr("XMODEM EOT handshake failed."));
    return false;
}

bool ModemThread::rxXModem(const TaskConfig& cfg)
{
    const auto txLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("TX"), bytes);
    };
    const auto rxLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("RX"), bytes);
    };

    QFile file(cfg.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred(tr("Failed to create file: %1").arg(file.errorString()));
        return false;
    }

    QSerialPort serial;
    serial.setPortName(cfg.portName);
    serial.setBaudRate(cfg.baudRate);
    serial.setDataBits(static_cast<QSerialPort::DataBits>(cfg.dataBits));
    serial.setParity(static_cast<QSerialPort::Parity>(cfg.parity));
    serial.setStopBits(static_cast<QSerialPort::StopBits>(cfg.stopBits));
    serial.setFlowControl(static_cast<QSerialPort::FlowControl>(cfg.flowControl));
    if (!serial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("Failed to open serial port: %1").arg(serial.errorString()));
        return false;
    }

    const auto serialGuard = qScopeGuard([&serial]() { disconnectDevice(serial); });

    serial.clear(QSerialPort::AllDirections);
    emit information(tr("XMODEM RX waiting sender..."));

    bool useCrc = true;

    quint8 expectedBlock = 1;
    qint64 received = 0;
    bool firstFrame = true;
    while (!isInterruptionRequested()) {
        char head = 0;
        if (firstFrame) {
            if (waitFirstFrameWithHandshake(serial, CCH, head, 30000, 1000, txLogger, rxLogger)) {
                useCrc = true;
                emit information(tr("XMODEM RX mode: CRC"));
            } else if (waitFirstFrameWithHandshake(
                           serial, NAK, head, 30000, 1000, txLogger, rxLogger)) {
                useCrc = false;
                emit information(tr("XMODEM RX mode: Checksum"));
            } else {
                emit errorOccurred(tr("XMODEM receive timeout."));
                return false;
            }

            firstFrame = false;
        } else if (!readByte(serial, head, 15000, rxLogger)) {
            emit errorOccurred(tr("XMODEM receive timeout."));
            return false;
        }

        if (head == EOT) {
            writeAll(serial, QByteArray(1, ACK), 1000, txLogger);
            emit progressChanged(100);
            return true;
        }

        if (head == CAN) {
            emit errorOccurred(tr("Transfer canceled by peer."));
            return false;
        }

        if (head != SOH && head != STX) {
            continue;
        }

        const int payloadSize = (head == SOH) ? 128 : 1024;
        QByteArray hdr;
        QByteArray payload;
        QByteArray tail;
        const int tailSize = useCrc ? 2 : 1;
        if (!readExact(serial, hdr, 2, 3000, rxLogger)
            || !readExact(serial, payload, payloadSize, 3000, rxLogger)
            || !readExact(serial, tail, tailSize, 3000, rxLogger)) {
            writeAll(serial, QByteArray(1, NAK), 1000, txLogger);
            continue;
        }

        const quint8 block = static_cast<quint8>(hdr.at(0));
        const quint8 invBlock = static_cast<quint8>(hdr.at(1));
        const bool checksumOk = useCrc ? validateXyFrameCrc(payload, tail)
                                       : validateXyFrameChecksum(payload, tail);
        if (invBlock != static_cast<quint8>(0xFF - block) || !checksumOk) {
            writeAll(serial, QByteArray(1, NAK), 1000, txLogger);
            continue;
        }

        // ACK lost on line can cause sender to retransmit the previous block.
        if (block == static_cast<quint8>(expectedBlock - 1)) {
            writeAll(serial, QByteArray(1, ACK), 1000, txLogger);
            continue;
        }

        if (block != expectedBlock) {
            writeAll(serial, QByteArray(1, NAK), 1000, txLogger);
            continue;
        }

        if (file.write(payload) != payload.size()) {
            emit errorOccurred(tr("Failed to write output file."));
            return false;
        }

        received += payload.size();
        if (received > 0) {
            // XMODEM RX has no known total length, keep value responsive and clamp below 99.
            emit progressChanged(qMin(99, static_cast<int>(received / 1024)));
        }

        writeAll(serial, QByteArray(1, ACK), 1000, txLogger);
        ++expectedBlock;
    }

    return false;
}

bool ModemThread::txYModem(const TaskConfig& cfg)
{
    const auto txLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("TX"), bytes);
    };
    const auto rxLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("RX"), bytes);
    };

    QFile file(cfg.filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(tr("Failed to open file: %1").arg(file.errorString()));
        return false;
    }

    QFileInfo fileInfo(cfg.filePath);

    QSerialPort serial;
    serial.setPortName(cfg.portName);
    serial.setBaudRate(cfg.baudRate);
    serial.setDataBits(static_cast<QSerialPort::DataBits>(cfg.dataBits));
    serial.setParity(static_cast<QSerialPort::Parity>(cfg.parity));
    serial.setStopBits(static_cast<QSerialPort::StopBits>(cfg.stopBits));
    serial.setFlowControl(static_cast<QSerialPort::FlowControl>(cfg.flowControl));
    if (!serial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("Failed to open serial port: %1").arg(serial.errorString()));
        return false;
    }

    const auto serialGuard = qScopeGuard([&serial]() { disconnectDevice(serial); });

    char req = 0;
    bool syncOk = false;
    for (int i = 0; i < 30 && !isInterruptionRequested(); ++i) {
        if (readByte(serial, req, 1000, rxLogger) && req == CCH) {
            syncOk = true;
            break;
        }
    }

    if (!syncOk || isInterruptionRequested()) {
        emit errorOccurred(tr("YMODEM sync timeout."));
        return false;
    }

    QByteArray header;
    const QByteArray nameBytes = fileInfo.fileName().toUtf8();
    const QByteArray sizeBytes = QByteArray::number(file.size());
    header.append(nameBytes);
    header.append('\0');
    header.append(sizeBytes);
    const QByteArray block0 = buildXyFrame(false, 0, header);
    if (!writeAll(serial, block0, 2000, txLogger)) {
        emit errorOccurred(tr("Failed to send YMODEM header."));
        return false;
    }

    char r0 = 0;
    if (!readByte(serial, r0, 5000, rxLogger) || r0 != ACK) {
        emit errorOccurred(tr("YMODEM header not acknowledged."));
        return false;
    }

    char c = 0;
    if (!readByte(serial, c, 5000, rxLogger) || c != CCH) {
        emit errorOccurred(tr("YMODEM receiver not ready for data."));
        return false;
    }

    const qint64 total = qMax<qint64>(1, file.size());
    qint64 sent = 0;
    quint8 block = 1;
    while (!isInterruptionRequested()) {
        const QByteArray data = file.read(1024);
        if (data.isEmpty()) {
            break;
        }

        const QByteArray frame = buildXyFrame(true, block, data);
        bool acked = false;
        for (int retry = 0; retry < 10 && !isInterruptionRequested(); ++retry) {
            if (!writeAll(serial, frame, 2000, txLogger)) {
                continue;
            }

            char reply = 0;
            if (readByte(serial, reply, 5000, rxLogger) && reply == ACK) {
                acked = true;
                break;
            }
        }

        if (!acked || isInterruptionRequested()) {
            emit errorOccurred(tr("YMODEM frame not acknowledged."));
            return false;
        }

        sent += data.size();
        emit progressChanged(static_cast<int>((sent * 100) / total));
        ++block;
    }

    if (!writeAll(serial, QByteArray(1, EOT), 1000, txLogger)) {
        emit errorOccurred(tr("Failed to send YMODEM EOT."));
        return false;
    }

    char eotAck = 0;
    if (!readByte(serial, eotAck, 5000, rxLogger) || (eotAck != ACK && eotAck != CCH)) {
        emit errorOccurred(tr("YMODEM EOT handshake failed."));
        return false;
    }

    const QByteArray last = buildXyFrame(false, 0, QByteArray());
    if (!writeAll(serial, last, 2000, txLogger)) {
        emit errorOccurred(tr("Failed to send YMODEM ending block."));
        return false;
    }

    char endAck = 0;
    if (!readByte(serial, endAck, 5000, rxLogger) || endAck != ACK) {
        emit errorOccurred(tr("YMODEM ending block not acknowledged."));
        return false;
    }

    emit progressChanged(100);
    return true;
}

bool ModemThread::rxYModem(const TaskConfig& cfg)
{
    const auto txLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("TX"), bytes);
    };
    const auto rxLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("RX"), bytes);
    };

    QSerialPort serial;
    serial.setPortName(cfg.portName);
    serial.setBaudRate(cfg.baudRate);
    serial.setDataBits(static_cast<QSerialPort::DataBits>(cfg.dataBits));
    serial.setParity(static_cast<QSerialPort::Parity>(cfg.parity));
    serial.setStopBits(static_cast<QSerialPort::StopBits>(cfg.stopBits));
    serial.setFlowControl(static_cast<QSerialPort::FlowControl>(cfg.flowControl));
    if (!serial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("Failed to open serial port: %1").arg(serial.errorString()));
        return false;
    }

    const auto serialGuard = qScopeGuard([&serial]() { disconnectDevice(serial); });

    serial.clear(QSerialPort::AllDirections);
    emit information(tr("YMODEM RX waiting sender..."));

    char head = 0;
    if (!waitFirstFrameWithHandshake(serial, CCH, head, 60000, 1000, txLogger, rxLogger)
        || (head != SOH && head != STX)) {
        emit errorOccurred(tr("YMODEM header timeout."));
        return false;
    }

    const int headerSize = (head == SOH) ? 128 : 1024;
    QByteArray hdr;
    QByteArray payload;
    QByteArray crc;
    if (!readExact(serial, hdr, 2, 3000, rxLogger)
        || !readExact(serial, payload, headerSize, 3000, rxLogger)
        || !readExact(serial, crc, 2, 3000, rxLogger)) {
        emit errorOccurred(tr("YMODEM header read failed."));
        return false;
    }

    if (hdr.at(0) != 0 || static_cast<quint8>(hdr.at(1)) != 0xFF
        || !validateXyFrameCrc(payload, crc)) {
        emit errorOccurred(tr("Invalid YMODEM header frame."));
        return false;
    }

    const int zeroPos = payload.indexOf('\0');
    const QByteArray nameBytes = zeroPos >= 0 ? payload.left(zeroPos) : QByteArray();
    const QByteArray metaBytes = zeroPos >= 0 ? payload.mid(zeroPos + 1) : QByteArray();
    const QByteArray sizeBytes = metaBytes.left(
        metaBytes.indexOf(' ') >= 0 ? metaBytes.indexOf(' ') : metaBytes.indexOf('\0'));

    const qint64 expectedSize = QString::fromLatin1(sizeBytes).trimmed().toLongLong();
    QString outputPath = cfg.filePath;
    QFileInfo outInfo(outputPath);
    if (outInfo.exists() && outInfo.isDir()) {
        outputPath = outInfo.absoluteFilePath() + QDir::separator() + QString::fromUtf8(nameBytes);
    }

    if (outputPath.trimmed().isEmpty()) {
        outputPath = QDir::homePath() + QDir::separator() + QString::fromUtf8(nameBytes);
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred(tr("Failed to create file: %1").arg(file.errorString()));
        return false;
    }

    emit information(tr("YMODEM output: %1").arg(outputPath));
    if (!writeAll(serial, QByteArray(1, ACK), 1000, txLogger)
        || !writeAll(serial, QByteArray(1, CCH), 1000, txLogger)) {
        emit errorOccurred(tr("Failed to reply to YMODEM header."));
        return false;
    }

    qint64 received = 0;
    quint8 expectedBlock = 1;
    while (!isInterruptionRequested()) {
        char ch = 0;
        if (!readByte(serial, ch, 15000, rxLogger)) {
            emit errorOccurred(tr("YMODEM receive timeout."));
            return false;
        }

        if (ch == EOT) {
            writeAll(serial, QByteArray(1, ACK), 1000, txLogger);
            if (expectedSize > 0 && file.size() > expectedSize) {
                file.resize(expectedSize);
            }

            emit progressChanged(100);
            return true;
        }

        if (ch != SOH && ch != STX) {
            continue;
        }

        const int payloadSize = (ch == SOH) ? 128 : 1024;
        QByteArray h;
        QByteArray body;
        QByteArray csum;
        if (!readExact(serial, h, 2, 3000, rxLogger)
            || !readExact(serial, body, payloadSize, 3000, rxLogger)
            || !readExact(serial, csum, 2, 3000, rxLogger)) {
            writeAll(serial, QByteArray(1, NAK), 1000, txLogger);
            continue;
        }

        const quint8 block = static_cast<quint8>(h.at(0));
        if (static_cast<quint8>(h.at(1)) != static_cast<quint8>(0xFF - block)
            || block != expectedBlock || !validateXyFrameCrc(body, csum)) {
            writeAll(serial, QByteArray(1, NAK), 1000, txLogger);
            continue;
        }

        qint64 toWrite = body.size();
        if (expectedSize > 0) {
            toWrite = qMin<qint64>(toWrite, expectedSize - received);
        }

        if (toWrite > 0 && file.write(body.constData(), toWrite) != toWrite) {
            emit errorOccurred(tr("Failed to write output file."));
            return false;
        }

        received += qMax<qint64>(0, toWrite);
        if (expectedSize > 0) {
            emit progressChanged(
                static_cast<int>((qMin(received, expectedSize) * 100) / expectedSize));
        } else {
            emit progressChanged(qMin(99, static_cast<int>(received / 1024)));
        }

        writeAll(serial, QByteArray(1, ACK), 1000, txLogger);
        ++expectedBlock;
    }

    return false;
}

bool ModemThread::txZModem(const TaskConfig& cfg)
{
    const auto txLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("TX"), bytes);
    };
    const auto rxLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("RX"), bytes);
    };

    QFile file(cfg.filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(tr("Failed to open file: %1").arg(file.errorString()));
        return false;
    }

    QSerialPort serial;
    serial.setPortName(cfg.portName);
    serial.setBaudRate(cfg.baudRate);
    serial.setDataBits(static_cast<QSerialPort::DataBits>(cfg.dataBits));
    serial.setParity(static_cast<QSerialPort::Parity>(cfg.parity));
    serial.setStopBits(static_cast<QSerialPort::StopBits>(cfg.stopBits));
    serial.setFlowControl(static_cast<QSerialPort::FlowControl>(cfg.flowControl));
    if (!serial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("Failed to open serial port: %1").arg(serial.errorString()));
        return false;
    }

    const auto serialGuard = qScopeGuard([&serial]() { disconnectDevice(serial); });

    emit information(tr("ZMODEM debug mode uses lightweight framed transfer."));
    const qint64 total = qMax<qint64>(1, file.size());
    qint64 sent = 0;
    while (!isInterruptionRequested()) {
        const QByteArray chunk = file.read(1024);
        if (chunk.isEmpty()) {
            break;
        }

        QByteArray frame;
        frame.append(ZPAD);
        frame.append(ZDLE);
        frame.append(ZDATA);

        QByteArray lenBytes;
        QDataStream lenStream(&lenBytes, QIODevice::WriteOnly);
        lenStream.setByteOrder(QDataStream::LittleEndian);
        lenStream << static_cast<quint32>(chunk.size());
        frame.append(lenBytes);
        frame.append(chunk);
        const quint16 crc = crc16XModem(chunk);
        frame.append(static_cast<char>(crc & 0xFF));
        frame.append(static_cast<char>((crc >> 8) & 0xFF));

        if (!writeAll(serial, frame, 3000, txLogger)) {
            emit errorOccurred(tr("Failed to send ZMODEM frame."));
            return false;
        }

        char ack = 0;
        if (!readByte(serial, ack, 5000, rxLogger) || ack != ACK) {
            emit errorOccurred(tr("ZMODEM frame not acknowledged."));
            return false;
        }

        sent += chunk.size();
        emit progressChanged(static_cast<int>((sent * 100) / total));
    }

    if (!writeAll(serial, QByteArray(1, EOT), 1000, txLogger)) {
        emit errorOccurred(tr("Failed to send ZMODEM EOT."));
        return false;
    }

    emit progressChanged(100);
    return true;
}

bool ModemThread::rxZModem(const TaskConfig& cfg)
{
    const auto txLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("TX"), bytes);
    };
    const auto rxLogger = [this](const QByteArray& bytes) {
        logTraffic(QStringLiteral("RX"), bytes);
    };

    QFile file(cfg.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit errorOccurred(tr("Failed to create file: %1").arg(file.errorString()));
        return false;
    }

    QSerialPort serial;
    serial.setPortName(cfg.portName);
    serial.setBaudRate(cfg.baudRate);
    serial.setDataBits(static_cast<QSerialPort::DataBits>(cfg.dataBits));
    serial.setParity(static_cast<QSerialPort::Parity>(cfg.parity));
    serial.setStopBits(static_cast<QSerialPort::StopBits>(cfg.stopBits));
    serial.setFlowControl(static_cast<QSerialPort::FlowControl>(cfg.flowControl));
    if (!serial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("Failed to open serial port: %1").arg(serial.errorString()));
        return false;
    }

    const auto serialGuard = qScopeGuard([&serial]() { disconnectDevice(serial); });

    emit information(tr("ZMODEM debug mode expects lightweight framed transfer."));
    qint64 received = 0;
    while (!isInterruptionRequested()) {
        char head = 0;
        if (!readByte(serial, head, 15000, rxLogger)) {
            emit errorOccurred(tr("ZMODEM receive timeout."));
            return false;
        }

        if (head == EOT) {
            emit progressChanged(100);
            return true;
        }

        if (head != ZPAD) {
            continue;
        }

        QByteArray hdr;
        if (!readExact(serial, hdr, 2, 3000, rxLogger)) {
            continue;
        }

        if (hdr.at(0) != ZDLE || hdr.at(1) != ZDATA) {
            continue;
        }

        QByteArray lenBytes;
        if (!readExact(serial, lenBytes, 4, 3000, rxLogger)) {
            continue;
        }

        QDataStream lenStream(&lenBytes, QIODevice::ReadOnly);
        lenStream.setByteOrder(QDataStream::LittleEndian);
        quint32 payloadSize = 0;
        lenStream >> payloadSize;
        if (payloadSize == 0 || payloadSize > 1024 * 1024) {
            continue;
        }

        QByteArray payload;
        QByteArray crcBytes;
        if (!readExact(serial, payload, static_cast<int>(payloadSize), 3000, rxLogger)
            || !readExact(serial, crcBytes, 2, 3000, rxLogger)) {
            continue;
        }

        const quint16 crc = static_cast<quint16>(static_cast<quint8>(crcBytes.at(0))
                                                 | (static_cast<quint8>(crcBytes.at(1)) << 8));
        if (crc16XModem(payload) != crc) {
            writeAll(serial, QByteArray(1, NAK), 1000, txLogger);
            continue;
        }

        if (file.write(payload) != payload.size()) {
            emit errorOccurred(tr("Failed to write output file."));
            return false;
        }

        received += payload.size();
        emit progressChanged(qMin(99, static_cast<int>(received / 1024)));
        writeAll(serial, QByteArray(1, ACK), 1000, txLogger);
    }

    return false;
}
