/***************************************************************************************************
 * Copyright 2026-2026 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include <QThread>

#include <QMutex>
#include <QString>

class ModemThread : public QThread
{
    Q_OBJECT
public:
    enum class WorkMode { Rx, Tx };
    Q_ENUM(WorkMode);

    enum class Protocol { xModem, yModem, zModem };
    Q_ENUM(Protocol);

    struct TaskConfig
    {
        QString portName;
        qint32 baudRate{115200};
        int dataBits{8};
        int parity{0};
        int stopBits{1};
        int flowControl{0};
        WorkMode workMode{WorkMode::Tx};
        Protocol protocol{Protocol::xModem};
        QString filePath;
    };

public:
    explicit ModemThread(QObject* parent = nullptr);
    ~ModemThread() override;

    void setTaskConfig(const TaskConfig& config);
    void stopTransfer();

signals:
    void information(const QString& text);
    void errorOccurred(const QString& text);
    void progressChanged(int percent);
    void transferFinished(bool ok, const QString& message);

protected:
    void run() override;

private:
    bool runTransfer();
    bool runTx(const TaskConfig& cfg);
    bool runRx(const TaskConfig& cfg);
    void logTraffic(const QString& direction, const QByteArray& bytes);

    bool txXModem(const TaskConfig& cfg);
    bool rxXModem(const TaskConfig& cfg);
    bool txYModem(const TaskConfig& cfg);
    bool rxYModem(const TaskConfig& cfg);
    bool txZModem(const TaskConfig& cfg);
    bool rxZModem(const TaskConfig& cfg);

private:
    mutable QMutex m_configMutex;
    TaskConfig m_config;
};