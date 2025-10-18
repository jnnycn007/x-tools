﻿/***************************************************************************************************
 * Copyright 2025-2025 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xModbus project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include <QWidget>

namespace Ui {
class xMqtt;
}

namespace xMqtt {
class xMqtt : public QWidget
{
    Q_OBJECT
public:
    explicit xMqtt(QWidget* parent = nullptr);
    ~xMqtt();

private:
    Ui::xMqtt* ui;
};
} // namespace xMqtt