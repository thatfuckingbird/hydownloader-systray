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

#if __GNUC__ > 10
#include <source_location>
#endif
#include "hydownloaderjsonobjectlistmodel.h"

class HyDownloaderConnection;

class HyDownloaderSubscriptionModel : public HyDownloaderJSONObjectListModel
{
    Q_OBJECT

public:
    HyDownloaderSubscriptionModel();
    void setUpConnections(HyDownloaderConnection* oldConnection) override;
    std::uint64_t addOrUpdateObjects(const QJsonArray& objs) override;
    void refresh() override;

private slots:
    void handleSubscriptionData(std::uint64_t requestID, const QJsonArray& data);
};
