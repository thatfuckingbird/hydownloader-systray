/*
hydownloader-systray
Copyright (C) 2021  thatfuckingbird

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "datetimeformatdelegate.h"
#include <QVariant>
#include <QDateTime>
#include <QDateTimeEdit>

QString DateTimeFormatDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    switch(value.userType()) {
        case QMetaType::Type::QDateTime:
            return locale.toString(value.toDateTime(), "yyyy-MM-dd HH:MM:ss");
        default:
            return QStyledItemDelegate::displayText(value, locale);
    }
}

QWidget* DateTimeFormatDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
    if(strcmp(widget->metaObject()->className(), "QDateTimeEdit") == 0)
        dynamic_cast<QDateTimeEdit*>(widget)->setDisplayFormat("yyyy-MM-dd HH:MM:ss");
    return widget;
}
