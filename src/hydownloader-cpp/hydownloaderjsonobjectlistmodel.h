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

#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QHash>

class HyDownloaderConnection;

class HyDownloaderJSONObjectListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    using ColumnData = QVector<std::tuple<QString, QString, bool, std::function<QVariant(const QJsonValue&)>, std::function<QJsonValue(const QVariant&)>>>;
    HyDownloaderJSONObjectListModel(ColumnData&& columnData) :
        m_columnData{columnData} {};
    void setConnection(HyDownloaderConnection* connection);
    virtual void setUpConnections(HyDownloaderConnection* oldConnection) = 0;
    virtual void refresh() = 0;
    virtual std::uint64_t addOrUpdateObject(const QJsonObject& obj) = 0;
    QVector<int> getIDs(const QModelIndexList& indices) const;
    void clear();
    int columnCount(const QModelIndex& parent) const override;
    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    static constexpr auto toVariant = [](const QJsonValue& val) {
        return val.toVariant();
    };
    static constexpr auto toBool = [](const QJsonValue& val) {
        return static_cast<bool>(val.toInt());
    };
    static constexpr auto toDateTime = [](const QJsonValue& val) {
        return val.isNull() || val.isUndefined() ? QVariant{} : QDateTime::fromMSecsSinceEpoch(val.toDouble() * 1000);
    };
    static constexpr auto fromBool = [](const QVariant& val) {
        return static_cast<int>(val.toBool());
    };
    static constexpr auto fromDateTime = [](const QVariant& val) {
        return !val.isValid() || val.isNull() ? QJsonValue::fromVariant(val) : static_cast<double>(val.toDateTime().toMSecsSinceEpoch()) / 1000;
    };

private:
    QSet<std::uint64_t> m_updateIDs;

private slots:
    void handleReplyReceived(std::uint64_t requestID, const QJsonObject&);

protected:
    const ColumnData m_columnData;
    QJsonArray m_data;
    HyDownloaderConnection* m_connection = nullptr;
};
