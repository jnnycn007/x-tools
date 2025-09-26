﻿/***************************************************************************************************
 * Copyright 2025-2025 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of eTools project.
 *
 * eTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "datarecordsview.h"
#include "ui_datarecordsview.h"

#include <QMessageBox>

#include "common/iconengine.h"
#include "common/xtools.h"
#include "datarecordsmodel.h"
#include "datarecordsmodelfilter.h"

struct DataRecordsViewParameterKeys
{
    const QString type{"type"};
    const QString flag{"flag"};
    const QString format{"format"};
    const QString records{"records"};
    const QString searchText{"searchText"};
};

DataRecordsView::DataRecordsView(QWidget *parent)
    : Panel{parent}
    , ui{new Ui::DataRecordsView}
    , m_model{nullptr}
    , m_modelFilter{nullptr}
{
    ui->setupUi(this);
    ui->comboBoxTypes->addItem(tr("All"), DataRecordsModel::DataRecordsModelItemTypeAll);
    ui->comboBoxTypes->addItem(QString("Rx"), DataRecordsModel::DataRecordsModelItemTypeRx);
    ui->comboBoxTypes->addItem(QString("Tx"), DataRecordsModel::DataRecordsModelItemTypeTx);
    ui->comboBoxFlags->addItem(tr("All"), QString());
    setupTextFormat(ui->comboBoxFormats);
    ui->comboBoxRecords->addItem(tr("No Limit"), -1);
    ui->comboBoxRecords->addItem(QString("100"), 100);
    ui->comboBoxRecords->addItem(QString("1000"), 1000);
    ui->comboBoxRecords->addItem(QString("10000"), 10000);
    ui->comboBoxRecords->addItem(QString("100000"), 100000);
    ui->toolButtonClear->setIcon(xIcon(":/res/icons/cleaning_services.svg"));

    m_model = new DataRecordsModel(this);
    m_modelFilter = new DataRecordsModelFilter(this);
    m_modelFilter->setSourceModel(m_model);
    ui->tableView->setModel(m_modelFilter);

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->setAlternatingRowColors(true);

    connect(ui->comboBoxTypes, xComboBoxActivated, this, &DataRecordsView::onTypeChanged);
    connect(ui->comboBoxFlags, xComboBoxActivated, this, &DataRecordsView::onFlagChanged);
    connect(ui->comboBoxFormats, xComboBoxActivated, this, &DataRecordsView::onFormatChanged);
    connect(ui->comboBoxRecords, xComboBoxActivated, this, &DataRecordsView::onRecordsCountChanged);
    connect(ui->toolButtonClear, &QToolButton::clicked, this, &DataRecordsView::onClear);
    connect(ui->lineEditData, &QLineEdit::textChanged, this, &DataRecordsView::onSearchTextChanged);

    onTypeChanged();
    onFlagChanged();
    onFormatChanged();
    onRecordsCountChanged();
    onSearchTextChanged(ui->lineEditData->text());
}

DataRecordsView::~DataRecordsView()
{
    delete ui;
}

void DataRecordsView::onBytesRead(const QByteArray &bytes, const QString &flag)
{
    m_model->addRxItem(flag, bytes);
    tryAddFlag(flag);
}

void DataRecordsView::onBytesWritten(const QByteArray &bytes, const QString &flag)
{
    m_model->addTxItem(flag, bytes);
    tryAddFlag(flag);
}

void DataRecordsView::load(const QVariantMap &parameters) {}

QVariantMap DataRecordsView::save() const
{
    return QVariantMap{};
}

void DataRecordsView::tryAddFlag(const QString &flag)
{
    if (ui->comboBoxFlags->findData(flag) == -1) {
        ui->comboBoxFlags->addItem(flag, flag);
    }
}

void DataRecordsView::onTypeChanged()
{
    m_modelFilter->setType(ui->comboBoxTypes->currentData().toInt());
}

void DataRecordsView::onFlagChanged()
{
    m_modelFilter->setFlag(ui->comboBoxFlags->currentData().toString());
}

void DataRecordsView::onFormatChanged()
{
    m_model->setFormat(ui->comboBoxFormats->currentData().toInt());
}

void DataRecordsView::onClear()
{
    auto btn = QMessageBox::question(this,
                                     tr("Clear Records"),
                                     tr("Are you sure to clear all records?"),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    if (btn == QMessageBox::Yes) {
        m_model->clear();

        for (int i = ui->comboBoxFlags->count() - 1; i > 0; --i) {
            ui->comboBoxFlags->removeItem(i);
        }
    }
}

void DataRecordsView::onRecordsCountChanged()
{
    m_model->setMaxRecords(ui->comboBoxRecords->currentData().toInt());
}

void DataRecordsView::onSearchTextChanged(const QString &text)
{
    m_modelFilter->setSearchText(text);
}