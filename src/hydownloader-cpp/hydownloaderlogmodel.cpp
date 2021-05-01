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

#include "hydownloaderlogmodel.h"
#include "hydownloaderconnection.h"
#include <QGuiApplication>
#include <QClipboard>

HyDownloaderLogModel::HyDownloaderLogModel()
{
}

void HyDownloaderLogModel::setConnection(HyDownloaderConnection* connection)
{
    if(m_connection) {
        disconnect(m_connection, &HyDownloaderConnection::staticDataReceived, this, &HyDownloaderLogModel::handleStaticData);
        disconnect(m_connection, &HyDownloaderConnection::networkError, this, &HyDownloaderLogModel::handleNetworkError);
    }
    m_connection = connection;
    connect(m_connection, &HyDownloaderConnection::staticDataReceived, this, &HyDownloaderLogModel::handleStaticData);
    connect(m_connection, &HyDownloaderConnection::networkError, this, &HyDownloaderLogModel::handleNetworkError);
}

void HyDownloaderLogModel::loadSubscriptionLog(int id, bool unsupportedURLs)
{
    if(!m_connection) return;
    clear();
    auto requestID = m_connection->requestStaticData(QString{"logs/subscription-%2-%1gallery-dl-old.txt"}.arg(unsupportedURLs ? "unsupported-urls-" : "", QString::number(id)));
    m_receivedData.push_back({requestID, {}});
    requestID = m_connection->requestStaticData(QString{"logs/subscription-%2-%1gallery-dl-latest.txt"}.arg(unsupportedURLs ? "unsupported-urls-" : "", QString::number(id)));
    m_receivedData.push_back({requestID, {}});

    m_lastLoadedLog = LogType::SubscriptionLog;
    m_lastLoadedLogParams = {id, unsupportedURLs};
}

void HyDownloaderLogModel::loadDaemonLog()
{
    if(!m_connection) return;
    clear();
    auto requestID = m_connection->requestStaticData("logs/daemon.txt");
    m_receivedData.push_back({requestID, {}});

    m_lastLoadedLog = LogType::DaemonLog;
    m_lastLoadedLogParams.clear();
}

void HyDownloaderLogModel::loadSingleURLQueueLog(int id, bool unsupportedURLs)
{
    if(!m_connection) return;
    clear();
    auto requestID = m_connection->requestStaticData(QString{"logs/single-urls-%2-%1gallery-dl-old.txt"}.arg(unsupportedURLs ? "unsupported-urls-" : "", QString::number(id)));
    m_receivedData.push_back({requestID, {}});
    requestID = m_connection->requestStaticData(QString{"logs/single-urls-%2-%1gallery-dl-latest.txt"}.arg(unsupportedURLs ? "unsupported-urls-" : "", QString::number(id)));
    m_receivedData.push_back({requestID, {}});

    m_lastLoadedLog = LogType::SingleURLQueueLog;
    m_lastLoadedLogParams = {id, unsupportedURLs};
}

void HyDownloaderLogModel::clear()
{
    beginResetModel();
    m_receivedData.clear();
    m_lines.clear();
    m_rawLines.clear();
    m_logLevels.clear();
    m_lastLoadedLog = LogType::NoLog;
    m_lastLoadedLogParams.clear();
    endResetModel();
}

void HyDownloaderLogModel::copyToClipboard(const QModelIndexList& indices)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString text;
    if(!indices.isEmpty()) {
        for(const auto& index: indices) {
            text += m_rawLines[index.row()] + "\n";
        }
    } else {
        text = m_rawLines.join("\n");
    }
    clipboard->setText(text);
}

void HyDownloaderLogModel::refresh()
{
    switch(m_lastLoadedLog) {
        case LogType::NoLog:
            clear();
            break;
        case LogType::SubscriptionLog:
            loadSubscriptionLog(m_lastLoadedLogParams[0].toInt(), m_lastLoadedLogParams[1].toBool());
            break;
        case LogType::DaemonLog:
            loadDaemonLog();
            break;
        case LogType::SingleURLQueueLog:
            loadSingleURLQueueLog(m_lastLoadedLogParams[0].toInt(), m_lastLoadedLogParams[1].toBool());
            break;
    }
}

int HyDownloaderLogModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int HyDownloaderLogModel::rowCount(const QModelIndex&) const
{
    return m_lines.size();
}

QVariant HyDownloaderLogModel::data(const QModelIndex& index, int role) const
{
    if(index.parent().isValid() || role != Qt::DisplayRole) return {};

    switch(index.column()) {
        case 0:
            switch(m_logLevels[index.row()]) {
                case LogLevel::Debug:
                    return "debug";
                case LogLevel::Info:
                    return "info";
                case LogLevel::Warning:
                    return "warning";
                case LogLevel::Error:
                    return "error";
                case LogLevel::Fatal:
                    return "fatal";
                case LogLevel::Unknown:
                    return "unknown";
            }
        case 1:
            return m_lines[index.row()];
        default:
            return {};
    }
}

QVariant HyDownloaderLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};

    switch(section) {
        case 0:
            return "Log level";
        case 1:
            return "Message";
        default:
            return {};
    }
}

void HyDownloaderLogModel::handleStaticData(uint64_t requestID, const QByteArray& data)
{
    bool allDataReceived = true;
    std::int64_t lineCount = 0;
    for(auto& [id, d]: m_receivedData) {
        if(id == requestID) {
            id = 0;
            d = QString{data}.trimmed().split('\n', Qt::SkipEmptyParts);
        }
        lineCount += d.size();
        if(id != 0) allDataReceived = false;
    }

    if(!allDataReceived) return;

    beginInsertRows({}, 0, lineCount - 1);
    m_lines.reserve(lineCount);
    m_rawLines.reserve(lineCount);
    m_logLevels.reserve(lineCount);
    lineCount = 0;
    for([[maybe_unused]] auto& [id, d]: m_receivedData) {
        for(const auto& line: d) {
            auto&& [finalLine, logLevel] = processLine(line);
            m_rawLines.append(line.trimmed());
            m_lines.append(finalLine);
            m_logLevels.append(logLevel);
            lineCount++;
            if(lineCount & 2048) QCoreApplication::processEvents();
        }
    }
    endInsertRows();
    m_receivedData.clear();
}

void HyDownloaderLogModel::handleNetworkError(uint64_t requestID, int status, QNetworkReply::NetworkError, const QString&)
{
    if(status == 404) {
        handleStaticData(requestID, {});
    }
}

std::pair<QString, HyDownloaderLogModel::LogLevel> HyDownloaderLogModel::processLine(const QString& line)
{
    if(line.startsWith("INFO ")) {
        return {line.mid(5), LogLevel::Info};
    }
    if(line.startsWith("WARNING ")) {
        return {line.mid(8), LogLevel::Warning};
    }
    if(line.startsWith("ERROR ")) {
        return {line.mid(6), LogLevel::Error};
    }
    if(line.startsWith("DEBUG ")) {
        return {line.mid(6), LogLevel::Debug};
    }
    if(line.startsWith("FATAL ")) {
        return {line.mid(6), LogLevel::Fatal};
    }

    if(line.contains("[debug]")) {
        return {QString{line}.replace("[debug]", ""), LogLevel::Debug};
    }
    if(line.contains("[info]")) {
        return {QString{line}.replace("[info]", ""), LogLevel::Info};
    }
    if(line.contains("[warning]")) {
        return {QString{line}.replace("[warning]", ""), LogLevel::Warning};
    }
    if(line.contains("[error]")) {
        return {QString{line}.replace("[error]", ""), LogLevel::Error};
    }
    if(line.contains("[fatal]")) {
        return {QString{line}.replace("[fatal]", ""), LogLevel::Fatal};
    }

    return {line, LogLevel::Unknown};
}
