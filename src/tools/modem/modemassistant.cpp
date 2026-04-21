/***************************************************************************************************
 * Copyright 2026-2026 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "modemassistant.h"
#include "ui_modemassistant.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

#include "common/x.h"
#include "modemthread.h"

ModemAssistant::ModemAssistant(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ModemAssistant)
{
    ui->setupUi(this);
    m_modemThread = new ModemThread(this);

    xSetupPortName(ui->comboBoxPortName);
    xSetupBaudRate(ui->comboBoxBaudRate);
    xSetupDataBits(ui->comboBoxDataBits);
    xSetupParity(ui->comboBoxParity);
    xSetupStopBits(ui->comboBoxStopBits);
    xSetupFlowControl(ui->comboBoxFlowControl);

    ui->comboBoxWorkMode->addItem(tr("Receive"), QVariant::fromValue(ModemThread::WorkMode::Rx));
    ui->comboBoxWorkMode->addItem(tr("Send"), QVariant::fromValue(ModemThread::WorkMode::Tx));
    ui->comboBoxProtocol->addItem(tr("X-Modem"), QVariant::fromValue(ModemThread::Protocol::xModem));
    ui->comboBoxProtocol->addItem(tr("Y-Modem"), QVariant::fromValue(ModemThread::Protocol::yModem));
    ui->comboBoxProtocol->addItem(tr("Z-Modem"), QVariant::fromValue(ModemThread::Protocol::zModem));

    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat(QStringLiteral("Idle"));
    ui->textBrowser->document()->setMaximumBlockCount(2000);
    ui->pushButtonStop->setEnabled(false);

    connect(ui->pushButtonStart, &QPushButton::clicked, this, &ModemAssistant::onStartClicked);
    connect(ui->pushButtonStop, &QPushButton::clicked, this, &ModemAssistant::onStopClicked);
    connect(ui->pushButtonBrowse, &QPushButton::clicked, this, &ModemAssistant::onBrowseClicked);
    connect(ui->comboBoxWorkMode,
            &QComboBox::currentIndexChanged,
            this,
            &ModemAssistant::onWorkModeChanged);

    connect(m_modemThread, &ModemThread::started, this, [=]() { updateUiState(true); });
    connect(m_modemThread, &ModemThread::finished, this, [=]() { updateUiState(false); });
    connect(m_modemThread, &ModemThread::information, this, [=](const QString& text) {
        appendInformation(text, false);
    });
    connect(m_modemThread, &ModemThread::errorOccurred, this, [=](const QString& text) {
        appendInformation(text, true);
    });
    connect(m_modemThread, &ModemThread::progressChanged, this, [=](int value) {
        int percent = qBound(0, value, 100);
        ui->progressBar->setValue(percent);
        ui->progressBar->setFormat(QStringLiteral("%1%").arg(percent));
    });
    connect(m_modemThread,
            &ModemThread::transferFinished,
            this,
            [=](bool ok, const QString& message) {
                if (ok) {
                    ui->progressBar->setValue(100);
                    ui->progressBar->setFormat(tr("Done"));
                } else if (ui->progressBar->value() == 0) {
                    ui->progressBar->setFormat(tr("Failed"));
                }

                appendInformation(message, !ok);
            });

    onWorkModeChanged();
}

ModemAssistant::~ModemAssistant()
{
    if (m_modemThread && m_modemThread->isRunning()) {
        m_modemThread->stopTransfer();
        m_modemThread->wait();
    }

    delete ui;
}

void ModemAssistant::onStartClicked()
{
    if (!m_modemThread || m_modemThread->isRunning()) {
        return;
    }

    ModemThread::TaskConfig config = buildTaskConfig();
    if (config.portName.trimmed().isEmpty()) {
        appendInformation(tr("Invalid serial port."), true);
        return;
    }

    if (config.filePath.trimmed().isEmpty()) {
        appendInformation(tr("Please choose a file first."), true);
        return;
    }

    if (config.workMode == ModemThread::WorkMode::Tx) {
        QFileInfo info(config.filePath);
        if (!info.exists() || !info.isFile()) {
            appendInformation(tr("The selected file does not exist."), true);
            return;
        }
    }

    ui->progressBar->setValue(0);
    ui->progressBar->setFormat(tr("Running"));
    m_modemThread->setTaskConfig(config);
    m_modemThread->start();
}

void ModemAssistant::onStopClicked()
{
    if (!m_modemThread || !m_modemThread->isRunning()) {
        return;
    }

    appendInformation(tr("Stopping transfer..."));
    m_modemThread->stopTransfer();
}

void ModemAssistant::onBrowseClicked()
{
    ModemThread::WorkMode mode = ui->comboBoxWorkMode->currentData().value<ModemThread::WorkMode>();
    QString fileName;
    if (mode == ModemThread::WorkMode::Tx) {
        fileName = QFileDialog::getOpenFileName(this, tr("Select File"));
    } else {
        QString suggested = ui->lineEditFileName->text().trimmed();
        if (suggested.isEmpty()) {
            suggested = QDir::homePath() + QDir::separator() + QStringLiteral("modem_rx.bin");
        }

        fileName = QFileDialog::getSaveFileName(this, tr("Save File"), suggested);
    }

    if (!fileName.isEmpty()) {
        ui->lineEditFileName->setText(fileName);
    }
}

void ModemAssistant::onWorkModeChanged()
{
    ModemThread::WorkMode mode = ui->comboBoxWorkMode->currentData().value<ModemThread::WorkMode>();
    ui->lineEditFileName->setPlaceholderText(
        mode == ModemThread::WorkMode::Tx ? tr("Input file path") : tr("Output file path"));
}

void ModemAssistant::appendInformation(const QString& text, bool error)
{
    const QString prefix = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    QString line = QStringLiteral("[%1] %2").arg(prefix, text.toHtmlEscaped());
    if (error) {
        line = QStringLiteral("<font color=tomato>%1</font>").arg(line);
    }

    ui->textBrowser->append(line);
}

void ModemAssistant::updateUiState(bool running)
{
    ui->pushButtonStart->setEnabled(!running);
    ui->pushButtonStop->setEnabled(running);
    ui->pushButtonBrowse->setEnabled(!running);
    ui->comboBoxPortName->setEnabled(!running);
    ui->comboBoxBaudRate->setEnabled(!running);
    ui->comboBoxDataBits->setEnabled(!running);
    ui->comboBoxParity->setEnabled(!running);
    ui->comboBoxStopBits->setEnabled(!running);
    ui->comboBoxFlowControl->setEnabled(!running);
    ui->comboBoxWorkMode->setEnabled(!running);
    ui->comboBoxProtocol->setEnabled(!running);
    ui->lineEditFileName->setEnabled(!running);
}

ModemThread::TaskConfig ModemAssistant::buildTaskConfig() const
{
    ModemThread::TaskConfig config;
    config.portName = ui->comboBoxPortName->currentText().trimmed();
    config.baudRate = ui->comboBoxBaudRate->currentData().toInt();
    config.dataBits = ui->comboBoxDataBits->currentData().toInt();
    config.parity = ui->comboBoxParity->currentData().toInt();
    config.stopBits = ui->comboBoxStopBits->currentData().toInt();
    config.flowControl = ui->comboBoxFlowControl->currentData().toInt();
    config.workMode = ui->comboBoxWorkMode->currentData().value<ModemThread::WorkMode>();
    config.protocol = ui->comboBoxProtocol->currentData().value<ModemThread::Protocol>();
    config.filePath = ui->lineEditFileName->text().trimmed();
    return config;
}