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

#include <QObject>
#include <QSslConfiguration>
#include <QNetworkReply>

class QNetworkAccessManager;
class QNetworkReply;

class HyDownloaderConnection : public QObject
{
    Q_OBJECT

public:
    explicit HyDownloaderConnection(QObject* parent = nullptr);
    enum class RequestType {
        StaticData,
        StatusInformation,
        SubscriptionData,
        SingleURLQueueData,
        APIVersion
    };
    Q_ENUM(RequestType)

public slots:
    void setAPIURL(const QString& url);
    QString apiURL() const;
    void setAccessKey(const QString& key);
    QString accessKey() const;
    void setStrictTransportSecurityEnabled(bool enabled);
    bool isStrictTransportSecurityEnabled() const;
    void setCertificateVerificationEnabled(bool enabled);
    bool isCertificateVerificationEnabled() const;
    void setTransferTimeout(int timeout);
    int transferTimeout() const;
    std::uint64_t requestStaticData(QString filePath);
    std::uint64_t requestStatusInformation();
    std::uint64_t requestSubscriptionData();
    std::uint64_t requestSingleURLQueueData();
    std::uint64_t requestAPIVersion();
    std::uint64_t deleteURLs(const QVector<int>& ids);
    std::uint64_t deleteSubscriptions(const QVector<int>& ids);
    std::uint64_t addOrUpdateURLs(const QJsonArray& data);
    std::uint64_t addOrUpdateSubscriptions(const QJsonArray& data);
    std::uint64_t pauseSubscriptions();
    std::uint64_t resumeSubscriptions();
    std::uint64_t pauseSingleURLQueue();
    std::uint64_t resumeSingleURLQueue();
    std::uint64_t runTests(const QStringList& sites);
    std::uint64_t runReport(bool verbose);
    void shutdown();

signals:
    void sslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
    void networkError(std::uint64_t requestID, int status, QNetworkReply::NetworkError error, const QString& errorText);
    void staticDataReceived(std::uint64_t requestID, const QByteArray& data);
    void statusInformationReceived(std::uint64_t requestID, const QJsonObject& info);
    void subscriptionDataReceived(std::uint64_t requestID, const QJsonArray& data);
    void singleURLQueueDataReceived(std::uint64_t requestID, const QJsonArray& data);
    void apiVersionReceived(std::uint64_t requestID, int version);
    void replyReceived(std::uint64_t requestID, const QJsonObject& data);

private slots:
    void handleNetworkReplyFinished(QNetworkReply* reply);
    QNetworkReply* post(const QString& endpoint, const QJsonDocument& body);
    QNetworkReply* get(const QString& endpoint, const QMap<QString, QString>& args);

private:
    QNetworkAccessManager* m_nam = nullptr;
    QSslConfiguration m_sslConfig;
    QString m_apiURL;
    QString m_accessKey;
    std::atomic_uint64_t m_requestIDCounter = 0;
    QNetworkReply* setRequestID(QNetworkReply* reply);
};
