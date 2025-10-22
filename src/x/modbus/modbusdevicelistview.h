/***************************************************************************************************
 * Copyright 2025-2025 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include <QWidget>

namespace Ui {
class ModbusDeviceListView;
}

namespace xModbus {

class ModbusDevice;
class ModbusDeviceListModel;
class ModbusDeviceListView : public QWidget
{
    Q_OBJECT
public:
    explicit ModbusDeviceListView(QWidget *parent = nullptr);
    ~ModbusDeviceListView() override;

signals:
    void currentDeviceChanged(ModbusDevice *device);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    struct
    {
        QAction *device;

        QAction *coils;
        QAction *discreteInputs;
        QAction *holdingRegisters;
        QAction *inputRegisters;
        QAction *singleRegister;
        QAction *multiRegister;

        QAction *copy;
        QAction *paste;
        QAction *cut;

        QAction *redo;
        QAction *undo;

        QAction *remove;
    } m_addActions;

private:
    Ui::ModbusDeviceListView *ui;
    ModbusDeviceListModel *m_model;

private:
    int depth(const QModelIndex &index);

    void onNewDevice();
    void onNewCoils();
    void onNewDiscreteInputs();
    void onNewHoldingRegisters();
    void onNewInputRegisters();
    void onNewSingleRegister();
    void onNewMultiRegister();
    void onCopy();
    void onPaste();
    void onCut();
    void onUndo();
    void onRedo();
    void onRemove();
    void onItemDoubleClicked(const QModelIndex &index);
    void onAddMenuAboutToShow();
    void onAddMenuAboutToHide();
};

} // namespace xModbus