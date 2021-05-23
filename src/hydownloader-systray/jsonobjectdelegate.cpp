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

#include "jsonobjectdelegate.h"
#include <QVariant>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QComboBox>

QString JSONObjectDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    switch(value.userType()) {
        case QMetaType::Type::QDateTime:
            return locale.toString(value.toDateTime(), "yyyy-MM-dd hh:mm:ss");
        default:
            return QStyledItemDelegate::displayText(value, locale);
    }
}

QWidget* JSONObjectDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if(!itemLists.contains(index.column())) {
        QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
        if(strcmp(widget->metaObject()->className(), "QDateTimeEdit") == 0)
            dynamic_cast<QDateTimeEdit*>(widget)->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
        return widget;
    } else {
        QComboBox* comboBox = new QComboBox{parent};
        comboBox->addItems(itemLists[index.column()]);
        return comboBox;
    }
}

void JSONObjectDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QComboBox* comboBox = dynamic_cast<QComboBox*>(editor);
    if(comboBox) {
        comboBox->setCurrentIndex(comboBox->findText(index.data(Qt::EditRole).toString()) && itemLists.contains(index.column()));
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void JSONObjectDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QComboBox* comboBox = dynamic_cast<QComboBox*>(editor);
    if(comboBox && comboBox->currentIndex() >= 0 && itemLists.contains(index.column())) {
        model->setData(index, comboBox->currentText());
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void JSONObjectDelegate::setItemListForColumn(int column, const QStringList& items)
{
    itemLists[column] = items;
}
