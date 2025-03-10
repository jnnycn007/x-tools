/***************************************************************************************************
 * Copyright 2024-2025 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include <QWidget>

namespace Ui {
class FileMergeAssistant;
}

class FileMergeAssistant : public QWidget
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit FileMergeAssistant(QWidget *parent = nullptr);
    ~FileMergeAssistant();

private:
    Ui::FileMergeAssistant *ui;
    QString m_desktopPath;

private:
    void onImportPushButtonClicked();
    void onMergePushButtonClicked();
    void onClearPushButtonClicked();
    void onUpPushButtonClicked();
    void onDownPushButtonClicked();
    void onRemovePushButtonClicked();

    void setProgressBarRange();
};
