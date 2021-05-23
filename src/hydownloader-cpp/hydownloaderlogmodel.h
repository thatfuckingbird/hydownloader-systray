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
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool showOnlyLatest READ showOnlyLatest WRITE setShowOnlyLatest NOTIFY showOnlyLatestChanged)

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
    void loadStatusLog();
    void clear();
    void copyToClipboard(const QModelIndexList& indices = {});
    void refresh();

    int columnCount(const QModelIndex& parent) const override;
    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void addStatusLogLine(LogLevel level, const QString& text);

    Q_INVOKABLE QString statusText() const;
    Q_INVOKABLE bool showOnlyLatest() const;

public slots:
    void setShowOnlyLatest(bool onlyLatest);

signals:
    void statusTextChanged(const QString&);
    void showOnlyLatestChanged(bool);

private slots:
    void handleStaticData(std::uint64_t requestID, const QByteArray& data);
    void handleNetworkError(std::uint64_t requestID, int status, QNetworkReply::NetworkError, const QString&);

private:
    enum class LogType {
        NoLog,
        SubscriptionLog,
        DaemonLog,
        SingleURLQueueLog,
        StatusLog
    };
    LogType m_lastLoadedLog = LogType::NoLog;
    QVariantList m_lastLoadedLogParams;
    QVector<std::pair<std::uint64_t, QStringList>> m_receivedData;
    QStringList m_lines;
    QStringList m_rawLines;
    QVector<LogLevel> m_logLevels;
    HyDownloaderConnection* m_connection = nullptr;
    QString m_statusText;
    std::pair<QString, LogLevel> processLine(const QString& line);
    QStringList m_statusLines;
    QStringList m_rawStatusLines;
    QVector<LogLevel> m_statusLogLevels;
    bool m_showOnlyLatest = false;
};
