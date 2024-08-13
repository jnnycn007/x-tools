﻿/***************************************************************************************************
 * Copyright 2024 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "TcpClientTransferModel.h"

#include "../Communication/UdpClient.h"

namespace xTools {

TcpClientTransferModel::TcpClientTransferModel(QObject *parent)
    : SocketTransferModel(parent)
{}

TcpClientTransferModel::~TcpClientTransferModel() {}

Socket *TcpClientTransferModel::createSocket()
{
    return new UdpClient(this);
}

} // namespace xTools