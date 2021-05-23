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

#include "hydownloaderjsonobjectlistmodel.h"

class HyDownloaderConnection;

class HyDownloaderSubscriptionChecksModel : public HyDownloaderJSONObjectListModel
{
    Q_OBJECT
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool showArchived READ showArchived NOTIFY showArchivedChanged WRITE setShowArchived)

public:
    HyDownloaderSubscriptionChecksModel();
    void setUpConnections(HyDownloaderConnection* oldConnection) override;
    std::uint64_t addOrUpdateObjects(const QJsonArray& objs) override;
    void refresh(bool full = true) override;
    void clear() override;

public slots:
    void loadDataForSubscription(int subscriptionID);
    QString statusText() const;
    bool showArchived() const;
    void setShowArchived(bool show);

signals:
    void statusTextChanged(const QString&);
    void showArchivedChanged(bool);

private slots:
    void handleSubscriptionChecksData(std::uint64_t requestID, const QJsonArray& data);

private:
    int m_lastRequestedID = 0;
    QString m_statusText;
    bool m_showArchived = false;
};
