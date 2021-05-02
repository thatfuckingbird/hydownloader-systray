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

#include "hydownloadersingleurlqueuemodel.h"
#include "hydownloaderconnection.h"
#include <QJsonObject>

HyDownloaderSingleURLQueueModel::HyDownloaderSingleURLQueueModel() :
    HyDownloaderJSONObjectListModel{
      {{"id", "ID", false, toVariant, &QJsonValue::fromVariant},
       {"url", "URL", true, toVariant, &QJsonValue::fromVariant},
       {"priority", "Priority", true, toVariant, &QJsonValue::fromVariant},
       {"ignore_anchor", "Ignore anchor", true, toBool, fromBool},
       {"additional_data", "Additional data", true, toVariant, &QJsonValue::fromVariant},
       {"status", "Status", true, toVariant, &QJsonValue::fromVariant},
       {"status_text", "Result status", false, toVariant, &QJsonValue::fromVariant},
       {"time_added", "Time added", true, toDateTime, fromDateTime},
       {"time_processed", "Time processed", false, toDateTime, fromDateTime},
       {"paused", "Paused", true, toBool, fromBool},
       {"metadata_only", "Metadata only", true, toBool, fromBool},
       {"overwrite_existing", "Overwrite existing", true, toBool, fromBool},
       {"filter", "Filter", true, toVariant, &QJsonValue::fromVariant},
       {"max_files", "Max files", true, toVariant, &QJsonValue::fromVariant},
       {"new_files", "New files", false, toVariant, &QJsonValue::fromVariant},
       {"already_seen_files", "Already seen files", false, toVariant, &QJsonValue::fromVariant},
       {"comment", "Comment", true, toVariant, &QJsonValue::fromVariant}}} {}

void HyDownloaderSingleURLQueueModel::setUpConnections(HyDownloaderConnection* oldConnection)
{
    if(oldConnection) disconnect(oldConnection, &HyDownloaderConnection::singleURLQueueDataReceived, this, &HyDownloaderSingleURLQueueModel::handleSingleURLQueueData);
    connect(m_connection, &HyDownloaderConnection::singleURLQueueDataReceived, this, &HyDownloaderSingleURLQueueModel::handleSingleURLQueueData);
}

std::uint64_t HyDownloaderSingleURLQueueModel::addOrUpdateObject(const QJsonObject& obj)
{
    return m_connection->addOrUpdateURLs({obj});
}

void HyDownloaderSingleURLQueueModel::refresh()
{
    clear();
    m_connection->requestSingleURLQueueData();
}

void HyDownloaderSingleURLQueueModel::handleSingleURLQueueData(uint64_t, const QJsonArray& data)
{
    clear();
    if(data.isEmpty()) return;
    beginInsertRows({}, 0, data.size() - 1);
    m_data = data;
    endInsertRows();
}
