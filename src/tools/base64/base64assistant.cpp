﻿/***************************************************************************************************
 * Copyright 2023-2025 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "base64assistant.h"
#include "ui_base64assistant.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>

Base64Assistant::Base64Assistant(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Base64Assistant)
{
    ui->setupUi(this);
    connect(ui->image, &QPushButton::clicked, this, &Base64Assistant::onImageClicked);
    connect(ui->decrypt, &QPushButton::clicked, this, &Base64Assistant::onDecryptClicked);
    connect(ui->encrypt, &QPushButton::clicked, this, &Base64Assistant::onEncryptClicked);
}

Base64Assistant::~Base64Assistant()
{
    delete ui;
}

void Base64Assistant::onImageClicked()
{
    QString cipherText = ui->cipherText->toPlainText();
    QByteArray base64 = cipherText.toUtf8();
    QByteArray bytes = QByteArray::fromBase64(base64);

    QPixmap pix;
    if (!pix.loadFromData(bytes)) {
        QMessageBox::warning(this, tr("Data error"), tr("Data can not convert image."));
        return;
    }

    QLabel* label = new QLabel(this);
    label->resize(pix.size());
    label->setPixmap(pix);

    QDialog dialog(this);
    dialog.setLayout(new QHBoxLayout());
    dialog.layout()->addWidget(label);
    dialog.setModal(true);
    dialog.exec();
}

void Base64Assistant::onEncryptClicked()
{
    QString plainText = ui->plainText->toPlainText();
    QByteArray byteArray = plainText.toUtf8();
    QByteArray base64 = byteArray.toBase64();
    QString ciperText = QString::fromLatin1(base64);
    ui->cipherText->setPlainText(ciperText);
}

void Base64Assistant::onDecryptClicked()
{
    QString cipherText = ui->cipherText->toPlainText();
    QByteArray base64 = cipherText.toUtf8();
    QByteArray byteArray = QByteArray::fromBase64(base64);
    QString plainText = QString::fromUtf8(byteArray);
    ui->plainText->setPlainText(plainText);
}
