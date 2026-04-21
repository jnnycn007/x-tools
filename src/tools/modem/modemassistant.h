/***************************************************************************************************
 * Copyright 2026-2026 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include <QWidget>

#include "modemthread.h"

namespace Ui {
class ModemAssistant;
}

class ModemAssistant : public QWidget
{
    Q_OBJECT
public:
    Q_INVOKABLE ModemAssistant(QWidget* parent = Q_NULLPTR);
    ~ModemAssistant();

private slots:
    void onStartClicked();
    void onStopClicked();
    void onBrowseClicked();
    void onWorkModeChanged();

private:
    void appendInformation(const QString& text, bool error = false);
    void updateUiState(bool running);
    ModemThread::TaskConfig buildTaskConfig() const;

private:
    Ui::ModemAssistant* ui;
    ModemThread* m_modemThread{Q_NULLPTR};
};