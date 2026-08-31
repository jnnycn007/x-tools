/***************************************************************************************************
 * Copyright 2026-2026 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xModbus is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "modbusdevicelistitemdelegate.h"

#include <QSize>

namespace xModbus {

ModbusDeviceListItemDelegate::ModbusDeviceListItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{ }

ModbusDeviceListItemDelegate::~ModbusDeviceListItemDelegate() { }

QSize ModbusDeviceListItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                             const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(26);
    return size;
}

} // namespace xModbus