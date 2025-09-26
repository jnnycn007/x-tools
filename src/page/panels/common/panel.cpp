﻿/***************************************************************************************************
 * Copyright 2025-2025 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of eTools project.
 *
 * eTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "panel.h"

Panel::Panel(QWidget *parent)
    : QWidget(parent)
{}

Panel::~Panel() {}

QVariantMap Panel::save() const
{
    return m_parameters;
}

void Panel::load(const QVariantMap &parameters)
{
    m_parameters = parameters;
}

QWidget *Panel::menuWidget()
{
    return nullptr;
}

QMenu *Panel::buttonMenu() const
{
    return nullptr;
}

void Panel::inputBytes(const QByteArray &bytes, const QString &flag)
{
    Q_UNUSED(bytes);
}

void Panel::onBytesRead(const QByteArray &bytes, const QString &flag)
{
    Q_UNUSED(bytes);
    Q_UNUSED(flag);
}

void Panel::onBytesWritten(const QByteArray &bytes, const QString &flag)
{
    Q_UNUSED(bytes);
    Q_UNUSED(flag);
}