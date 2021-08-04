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

#include "hydownloadersubscriptionmodel.h"
#include "hydownloaderconnection.h"

HyDownloaderSubscriptionModel::HyDownloaderSubscriptionModel() :
    HyDownloaderJSONObjectListModel{
      {{"id", "ID", false, toVariant, &QJsonValue::fromVariant},
       {"downloader", "Downloader", true, toVariant, &QJsonValue::fromVariant},
       {"keywords", "Keywords", true, toVariant, &QJsonValue::fromVariant},
       {"priority", "Priority", true, toVariant, &QJsonValue::fromVariant},
       {"paused", "Paused", true, toBool, fromBool},
       {"check_interval", "Check interval", true, toVariant, &QJsonValue::fromVariant},
       {"abort_after", "Abort after", true, toVariant, &QJsonValue::fromVariant},
       {"max_files_regular", "Max files (regular)", true, toVariant, &QJsonValue::fromVariant},
       {"max_files_initial", "Max files (initial)", true, toVariant, &QJsonValue::fromVariant},
       {"last_check", "Last check", false, toDateTime, fromDateTime},
       {"last_successful_check", "Last successful check", true, toDateTime, fromDateTime},
       {"additional_data", "Additional data", true, toVariant, &QJsonValue::fromVariant},
       {"time_created", "Time created", true, toDateTime, fromDateTime},
       {"filter", "Filter", true, toVariant, &QJsonValue::fromVariant},
       {"gallerydl_config", "gallery-dl config", true, toVariant, &QJsonValue::fromVariant},
       {"comment", "Comment", true, toVariant, &QJsonValue::fromVariant}},
      "id"} {}

void HyDownloaderSubscriptionModel::setUpConnections(HyDownloaderConnection* oldConnection)
{
    if(oldConnection) disconnect(oldConnection, &HyDownloaderConnection::subscriptionDataReceived, this, &HyDownloaderSubscriptionModel::handleSubscriptionData);
    connect(m_connection, &HyDownloaderConnection::subscriptionDataReceived, this, &HyDownloaderSubscriptionModel::handleSubscriptionData);
}

std::uint64_t HyDownloaderSubscriptionModel::addOrUpdateObjects(const QJsonArray& objs)
{
    return m_connection->addOrUpdateSubscriptions(objs);
}

void HyDownloaderSubscriptionModel::refresh(bool full)
{
    if(full) clear();
    m_connection->requestSubscriptionData();
}

void HyDownloaderSubscriptionModel::handleSubscriptionData(uint64_t, const QJsonArray& data)
{
    updateFromRowData(data);
}
