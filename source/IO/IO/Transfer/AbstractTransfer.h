/***************************************************************************************************
 * Copyright 2024 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include "../Model/AbstractModelIO.h"

namespace xTools {

class AbstractTransfer : public AbstractModelIO
{
    Q_OBJECT
public:
    AbstractTransfer(QObject *parent = nullptr);
    ~AbstractTransfer() override;

    void inputBytes(const QByteArray &bytes) override;

private:
    void onStarted();
    void onFinished();
};

} // namespace xTools