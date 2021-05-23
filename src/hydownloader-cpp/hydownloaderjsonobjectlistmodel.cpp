/*
hydownloader
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

#include "hydownloaderjsonobjectlistmodel.h"
#include "hydownloaderconnection.h"
#include <QJsonObject>

void HyDownloaderJSONObjectListModel::setConnection(HyDownloaderConnection* connection)
{
    auto oldConnection = m_connection;
    m_connection = connection;
    setUpConnections(oldConnection);
    if(oldConnection) disconnect(oldConnection, &HyDownloaderConnection::replyReceived, this, &HyDownloaderJSONObjectListModel::handleReplyReceived);
    connect(m_connection, &HyDownloaderConnection::replyReceived, this, &HyDownloaderJSONObjectListModel::handleReplyReceived);
    clear();
}

QVector<int> HyDownloaderJSONObjectListModel::getIDs(const QModelIndexList& indices) const
{
    QVector<int> res;
    res.reserve(indices.size());
    for(const auto& index: indices) {
        res.append(m_data[index.row()].toObject()["id"].toInt());
    }
    return res;
}

void HyDownloaderJSONObjectListModel::clear()
{
    beginResetModel();
    m_data = {};
    endResetModel();
}

int HyDownloaderJSONObjectListModel::columnCount(const QModelIndex& parent) const
{
    if(parent.isValid()) return 0;

    return m_columnData.size();
}

int HyDownloaderJSONObjectListModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid()) return 0;

    return m_data.size();
}

QVariant HyDownloaderJSONObjectListModel::data(const QModelIndex& index, int role) const
{
    if(index.parent().isValid() || (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole)) return {};

    const auto& conversionFunction = std::get<3>(m_columnData[index.column()]);
    const auto& key = std::get<0>(m_columnData[index.column()]);
    return conversionFunction(m_data[index.row()][key]);
}

bool HyDownloaderJSONObjectListModel::setData(const QModelIndex& index, const QVariant& value, int)
{
    if(index.parent().isValid()) return false;
    if(!std::get<2>(m_columnData[index.column()])) return false;

    const auto& key = std::get<0>(m_columnData[index.column()]);
    const auto& conversionFunction = std::get<4>(m_columnData[index.column()]);
    QJsonValue jsonValue = conversionFunction(value);
    QJsonObject tmp = m_data[index.row()].toObject();
    tmp[key] = jsonValue;
    m_data[index.row()] = tmp;

    QJsonObject updateObj;
    updateObj["id"] = m_data[index.row()].toObject()["id"];
    updateObj[key] = jsonValue;

    m_updateIDs.insert(addOrUpdateObjects({updateObj}));

    return true;
}

Qt::ItemFlags HyDownloaderJSONObjectListModel::flags(const QModelIndex& index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | (index.isValid() && std::get<2>(m_columnData[index.column()]) ? Qt::ItemIsEditable : Qt::NoItemFlags);
}

QVariant HyDownloaderJSONObjectListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(section < 0 || orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};

    return std::get<1>(m_columnData[section]);
}

QJsonObject HyDownloaderJSONObjectListModel::getRowData(const QModelIndex& rowIndex) const
{
    return m_data[rowIndex.row()].toObject();
}

void HyDownloaderJSONObjectListModel::setRowData(const QVector<QModelIndex>& indices, const QJsonArray& objs)
{
    if(indices.size() != objs.size() || indices.isEmpty()) return;
    int minRow = std::numeric_limits<int>::max();
    int maxRow = 0;
    for(int i = 0; i < indices.size(); ++i) {
        const int row = indices[i].row();
        m_data[row] = objs[i];
        if(row > maxRow) {
            maxRow = row;
        }
        if(row < minRow) {
            minRow = row;
        }
    }
    m_updateIDs.insert(addOrUpdateObjects(objs));
    emit dataChanged(createIndex(minRow, 0), createIndex(maxRow, m_columnData.size() - 1));
}

void HyDownloaderJSONObjectListModel::handleReplyReceived(uint64_t requestID, const QJsonDocument&)
{
    if(m_updateIDs.contains(requestID)) {
        m_updateIDs.remove(requestID);
        refresh(false);
    }
}

void HyDownloaderJSONObjectListModel::updateFromRowData(const QJsonArray& arr)
{
    if(arr.isEmpty()) {
        clear();
        return;
    }
    int commonPrefixLength = 0;
    for(; commonPrefixLength < arr.size() && commonPrefixLength < m_data.size(); ++commonPrefixLength) {
        if(arr[commonPrefixLength] != m_data[commonPrefixLength]) break;
    }
    int oldDataLength = m_data.size();
    int newDataLength = arr.size();
    if(oldDataLength < newDataLength) {
        beginInsertRows({}, oldDataLength, newDataLength - 1);
        m_data = arr;
        endInsertRows();
    } else if(oldDataLength > newDataLength) {
        beginRemoveRows({}, newDataLength, oldDataLength - 1);
        m_data = arr;
        endRemoveRows();
    } else {
        m_data = arr;
    }
    if(commonPrefixLength < m_data.size()) {
        emit dataChanged(createIndex(commonPrefixLength, 0), createIndex(m_data.size() - 1, m_columnData.size() - 1));
    }
}
