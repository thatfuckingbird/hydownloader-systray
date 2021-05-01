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
#include <QStringList>
#include <QNetworkReply>

class HyDownloaderConnection;

class HyDownloaderLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class LogLevel {
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
        Unknown
    };
    Q_ENUM(LogLevel)

    HyDownloaderLogModel();
    void setConnection(HyDownloaderConnection* connection);
    void loadSubscriptionLog(int id, bool unsupportedURLs = false);
    void loadDaemonLog();
    void loadSingleURLQueueLog(int id, bool unsupportedURLs = false);
    void clear();
    void copyToClipboard(const QModelIndexList& indices = {});
    void refresh();

    int columnCount(const QModelIndex& parent) const override;
    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private slots:
    void handleStaticData(std::uint64_t requestID, const QByteArray& data);
    void handleNetworkError(std::uint64_t requestID, int status, QNetworkReply::NetworkError, const QString&);

private:
    enum class LogType {
        NoLog,
        SubscriptionLog,
        DaemonLog,
        SingleURLQueueLog
    };
    LogType m_lastLoadedLog = LogType::NoLog;
    QVariantList m_lastLoadedLogParams;
    QVector<std::pair<std::uint64_t, QStringList>> m_receivedData;
    QStringList m_lines;
    QStringList m_rawLines;
    QVector<LogLevel> m_logLevels;
    HyDownloaderConnection* m_connection = nullptr;
    std::pair<QString, LogLevel> processLine(const QString& line);
};
