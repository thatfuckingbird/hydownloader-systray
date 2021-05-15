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

#include "hydownloadersubscriptionchecksmodel.h"
#include "hydownloaderconnection.h"
#include <QJsonObject>

HyDownloaderSubscriptionChecksModel::HyDownloaderSubscriptionChecksModel() :
    HyDownloaderJSONObjectListModel{
      {{"subscription_id", "Subscription ID", false, toVariant, &QJsonValue::fromVariant},
       {"time_started", "Time started", false, toDateTime, fromDateTime},
       {"time_finished", "Time finished", false, toDateTime, fromDateTime},
       {"status", "Result status", false, toVariant, &QJsonValue::fromVariant},
       {"new_files", "New files", false, toVariant, &QJsonValue::fromVariant},
       {"already_seen_files", "Already seen files", false, toVariant, &QJsonValue::fromVariant},
       {"archived", "Archived", true, toBool, fromBool}}},
    m_statusText{"No data loaded"} {}

void HyDownloaderSubscriptionChecksModel::setUpConnections(HyDownloaderConnection* oldConnection)
{
    if(oldConnection) disconnect(oldConnection, &HyDownloaderConnection::subscriptionChecksDataReceived, this, &HyDownloaderSubscriptionChecksModel::handleSubscriptionChecksData);
    connect(m_connection, &HyDownloaderConnection::subscriptionChecksDataReceived, this, &HyDownloaderSubscriptionChecksModel::handleSubscriptionChecksData);
    emit statusTextChanged(m_statusText);
}

std::uint64_t HyDownloaderSubscriptionChecksModel::addOrUpdateObjects(const QJsonArray& objs)
{
    return m_connection->addOrUpdateSubscriptionChecks(objs);
}

void HyDownloaderSubscriptionChecksModel::refresh()
{
    clear();
    if(m_lastRequestedID >= 0) {
        m_connection->requestSubscriptionChecksData(m_lastRequestedID, m_showArchived);
        m_statusText = "Loading subscription check data...";
        emit statusTextChanged(m_statusText);
    }
}

void HyDownloaderSubscriptionChecksModel::clear()
{
    HyDownloaderJSONObjectListModel::clear();
    m_statusText = "No data loaded";
    emit statusTextChanged(m_statusText);
}

void HyDownloaderSubscriptionChecksModel::loadDataForSubscription(int subscriptionID)
{
    m_lastRequestedID = subscriptionID;
    refresh();
}

QString HyDownloaderSubscriptionChecksModel::statusText() const
{
    return m_statusText;
}

bool HyDownloaderSubscriptionChecksModel::showArchived() const
{
    return m_showArchived;
}

void HyDownloaderSubscriptionChecksModel::setShowArchived(bool show)
{
    if(m_showArchived != show) {
        m_showArchived = show;
        emit showArchivedChanged(show);
    }
}

void HyDownloaderSubscriptionChecksModel::handleSubscriptionChecksData(uint64_t, const QJsonArray& data)
{
    clear();

    if(m_lastRequestedID) {
        m_statusText = QString{"Subscription check data for subscription %1"}.arg(m_lastRequestedID);
    } else {
        m_statusText = "Subscription check data for all subscriptions";
    }
    emit statusTextChanged(m_statusText);

    if(data.isEmpty()) return;
    beginInsertRows({}, 0, data.size() - 1);
    m_data = data;
    endInsertRows();
}
