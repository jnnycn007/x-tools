﻿/***************************************************************************************************
 * Copyright 2024 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of eTools project.
 *
 * eTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "UdpClientUi.h"

namespace xTools {

UdpClientUi::UdpClientUi(QWidget *parent)
    : SocketClientUi(xIO::CommunicationType::UdpClient, parent)
{
#if 1
    setClientWidgetsVisible(true);
#endif
    setWriteToWidgetsVisible(false);
    setChannelWidgetsVisible(false);
    setAuthenticationWidgetsVisible(false);
}

UdpClientUi::~UdpClientUi() {}

void UdpClientUi::setUiEnabled(bool enabled)
{
    setClientWidgetsEnabled(enabled);
    setServerWidgetsEnabled(enabled);
    setMulticastWidgetsEnabled(enabled);
}

} // namespace xTools
