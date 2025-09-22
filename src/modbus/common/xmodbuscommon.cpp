/***************************************************************************************************
 * Copyright 2025-2025 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xModbus project.
 *
 * xModbus is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "xmodbuscommon.h"

#include <QSerialPort>
#include <QSerialPortInfo>

namespace xModbus {

void setupModebusDeviceType(QComboBox *cb)
{
    cb->clear();
    cb->addItem(QObject::tr("RTU Master"), static_cast<int>(XModbusType::RtuClient));
    cb->addItem(QObject::tr("RTU Slave"), static_cast<int>(XModbusType::RtuServer));
    cb->addItem(QObject::tr("TCP Client"), static_cast<int>(XModbusType::TcpClient));
    cb->addItem(QObject::tr("TCP Server"), static_cast<int>(XModbusType::TcpServer));
}

void setupRtuNames(QComboBox *cb)
{
    cb->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        cb->addItem(port.portName(), port.portName());
    }
}

void setupRtuDataBits(QComboBox *cb)
{
    cb->clear();
    cb->addItem(QObject::tr("5"), QSerialPort::Data5);
    cb->addItem(QObject::tr("6"), QSerialPort::Data6);
    cb->addItem(QObject::tr("7"), QSerialPort::Data7);
    cb->addItem(QObject::tr("8"), QSerialPort::Data8);

    cb->setCurrentIndex(3);
}

void setupRtuParity(QComboBox *cb)
{
    cb->clear();
    cb->addItem(QObject::tr("None"), QSerialPort::NoParity);
    cb->addItem(QObject::tr("Even"), QSerialPort::EvenParity);
    cb->addItem(QObject::tr("Odd"), QSerialPort::OddParity);
    cb->addItem(QObject::tr("Space"), QSerialPort::SpaceParity);
    cb->addItem(QObject::tr("Mark"), QSerialPort::MarkParity);
}

void setupRtuStopBits(QComboBox *cb)
{
    cb->clear();
    cb->addItem(QObject::tr("1"), QSerialPort::OneStop);
#if defined(Q_OS_WIN)
    cb->addItem(QObject::tr("1.5"), QSerialPort::OneAndHalfStop);
#endif
    cb->addItem(QObject::tr("2"), QSerialPort::TwoStop);
}

void setupRtuBaudRate(QComboBox *cb)
{
    cb->clear();
    auto rates = QSerialPortInfo::standardBaudRates();
    for (const int &rate : std::as_const(rates)) {
        cb->addItem(QString::number(rate), rate);
    }

    cb->setCurrentText("9600");
}

QJsonObject deviceConnectionParameters2Json(const DeviceConnectionParameters &params)
{
    QJsonObject json;
    DeviceConnectionParameterKeys keys;

    json.insert(keys.deviceName, params.deviceName);
    json.insert(keys.deviceType, params.deviceType);
    json.insert(keys.portName, params.portName);
    json.insert(keys.dataBits, params.dataBits);
    json.insert(keys.parity, params.parity);
    json.insert(keys.stopBits, params.stopBits);
    json.insert(keys.baudRate, params.baudRate);
    json.insert(keys.tcpAddress, params.tcpAddress);
    json.insert(keys.tcpPort, params.tcpPort);
    json.insert(keys.numberOfRetries, params.numberOfRetries);
    json.insert(keys.timeout, params.timeout);
    json.insert(keys.serverAddress, params.serverAddress);
    json.insert(keys.listenOnlyMode, params.listenOnlyMode);

    return json;
}

DeviceConnectionParameters json2DeviceConnectionParameters(const QJsonObject &params)
{
    DeviceConnectionParameters connectionParams;
    DeviceConnectionParameterKeys keys;

    int defaultDeviceType = static_cast<int>(XModbusType::RtuClient);
    int defaultDataBits = QSerialPort::Data8;
    int defaultParity = QSerialPort::NoParity;
    int defaultStopBits = QSerialPort::OneStop;
    int defaultBaudRate = 9600;
    int defaultTcpPort = 502;
    int defaultRetries = 3;
    int defaultTimeout = 1000;
    int defaultServerAddress = 0;
    bool defaultListenOnlyMode = false;

    connectionParams.deviceName = params.value(keys.deviceName).toString("Untitled");
    connectionParams.deviceType = params.value(keys.deviceType).toInt(defaultDeviceType);
    connectionParams.portName = params.value(keys.portName).toString();
    connectionParams.dataBits = params.value(keys.dataBits).toInt(defaultDataBits);
    connectionParams.parity = params.value(keys.parity).toInt(defaultParity);
    connectionParams.stopBits = params.value(keys.stopBits).toInt(defaultStopBits);
    connectionParams.baudRate = params.value(keys.baudRate).toInt(defaultBaudRate);
    connectionParams.tcpAddress = params.value(keys.tcpAddress).toString();
    connectionParams.tcpPort = params.value(keys.tcpPort).toInt(defaultTcpPort);
    connectionParams.numberOfRetries = params.value(keys.numberOfRetries).toInt(defaultRetries);
    connectionParams.timeout = params.value(keys.timeout).toInt(defaultTimeout);
    connectionParams.serverAddress = params.value(keys.serverAddress).toInt(defaultServerAddress);
    connectionParams.listenOnlyMode = params.value(keys.listenOnlyMode).toBool(defaultListenOnlyMode);

    return connectionParams;
}

} // namespace xModbus