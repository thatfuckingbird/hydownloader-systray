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

#pragma once

#include <QStyledItemDelegate>

class JSONObjectDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit JSONObjectDelegate(QObject* parent = nullptr) :
        QStyledItemDelegate(parent) {}
    QString displayText(const QVariant& value, const QLocale& locale) const override;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void setItemListForColumn(int column, const QStringList& items);

private:
    QHash<int, QStringList> itemLists;
};
