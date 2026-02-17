#include "ApiClient.h"

#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
constexpr int kMaxApiRetries = 2;

bool shouldRetryRequest(QNetworkReply::NetworkError networkError, int httpStatus) {
    if (httpStatus == 429 || (httpStatus >= 500 && httpStatus <= 599)) {
        return true;
    }

    switch (networkError) {
    case QNetworkReply::TimeoutError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::ServiceUnavailableError:
    case QNetworkReply::UnknownNetworkError:
    case QNetworkReply::RemoteHostClosedError:
        return true;
    default:
        return false;
    }
}
}

ApiClient::ApiClient(QObject *parent)
    : QObject(parent) {}

QString ApiClient::baseUrl() const {
    return m_baseUrl;
}

void ApiClient::setBaseUrl(const QString &baseUrl) {
    if (m_baseUrl == baseUrl) {
        return;
    }
    m_baseUrl = baseUrl;
    emit baseUrlChanged();
}

QString ApiClient::accessToken() const {
    return m_accessToken;
}

QString ApiClient::refreshToken() const {
    return m_refreshToken;
}

void ApiClient::setAccessToken(const QString &token) {
    m_accessToken = token;
}

void ApiClient::setRefreshToken(const QString &token) {
    m_refreshToken = token;
}

void ApiClient::clearTokens() {
    m_accessToken.clear();
    m_refreshToken.clear();
}

void ApiClient::registerUser(const QString &email, const QString &password, const QString &displayName, const ObjectCallback &callback) {
    sendObjectRequest(
        "POST",
        "/auth/register",
        {
            {"email", email},
            {"password", password},
            {"displayName", displayName},
        },
        callback,
        false);
}

void ApiClient::login(const QString &email, const QString &password, const ObjectCallback &callback) {
    sendObjectRequest(
        "POST",
        "/auth/login",
        {
            {"email", email},
            {"password", password},
        },
        callback,
        false);
}

void ApiClient::refresh(const QString &refreshToken, const ObjectCallback &callback) {
    sendObjectRequest(
        "POST",
        "/auth/refresh",
        {
            {"refreshToken", refreshToken},
        },
        callback,
        false);
}

void ApiClient::me(const ObjectCallback &callback) {
    sendObjectRequest("GET", "/auth/me", {}, callback, true);
}

void ApiClient::updateProfile(const QString &displayName, const ObjectCallback &callback) {
    QJsonObject body;
    if (!displayName.isEmpty()) {
        body.insert("displayName", displayName);
    }
    sendObjectRequest("PATCH", "/users/me", body, callback);
}

void ApiClient::listGuilds(const ArrayCallback &callback) {
    sendArrayRequest("/guilds/me", callback);
}

void ApiClient::listGuildPresence(const QString &guildId, const ArrayCallback &callback) {
    sendArrayRequest(QString("/presence/guild/%1").arg(guildId), callback);
}

void ApiClient::createGuild(const QString &name, const ObjectCallback &callback) {
    sendObjectRequest("POST", "/guilds", { {"name", name} }, callback);
}

void ApiClient::listChannels(const QString &guildId, const ArrayCallback &callback) {
    sendArrayRequest(QString("/guilds/%1/channels").arg(guildId), callback);
}

void ApiClient::createChannel(const QString &guildId, const QString &name, const QString &type, const ObjectCallback &callback) {
    sendObjectRequest(
        "POST",
        QString("/guilds/%1/channels").arg(guildId),
        {
            {"name", name},
            {"type", type},
        },
        callback);
}

void ApiClient::createInvite(const QString &guildId, const ObjectCallback &callback) {
    sendObjectRequest("POST", QString("/guilds/%1/invites").arg(guildId), {}, callback);
}

void ApiClient::joinInvite(const QString &code, const ObjectCallback &callback) {
    sendObjectRequest("POST", QString("/guilds/join/%1").arg(code), {}, callback);
}

void ApiClient::listMessages(const QString &channelId, const QString &cursor, const QString &search, const ObjectCallback &callback) {
    QMap<QString, QString> query;
    if (!cursor.isEmpty()) {
        query.insert("cursor", cursor);
    }
    if (!search.isEmpty()) {
        query.insert("search", search);
    }
    sendObjectRequest("GET", QString("/channels/%1/messages").arg(channelId), {}, callback, true, query);
}

void ApiClient::sendMessage(const QString &channelId, const QString &content, const ObjectCallback &callback) {
    sendObjectRequest(
        "POST",
        QString("/channels/%1/messages").arg(channelId),
        {
            {"content", content},
        },
        callback);
}

void ApiClient::editMessage(const QString &channelId, const QString &messageId, const QString &content, const ObjectCallback &callback) {
    sendObjectRequest(
        "PATCH",
        QString("/channels/%1/messages/%2").arg(channelId, messageId),
        {
            {"content", content},
        },
        callback);
}

void ApiClient::deleteMessage(const QString &channelId, const QString &messageId, const ObjectCallback &callback) {
    sendObjectRequest("DELETE", QString("/channels/%1/messages/%2").arg(channelId, messageId), {}, callback);
}

void ApiClient::toggleReaction(const QString &channelId, const QString &messageId, const QString &emoji, const ObjectCallback &callback) {
    sendObjectRequest(
        "PUT",
        QString("/channels/%1/messages/%2/reactions/%3").arg(channelId, messageId, emoji),
        {},
        callback);
}

void ApiClient::sendTyping(const QString &channelId, const ObjectCallback &callback) {
    sendObjectRequest("POST", QString("/channels/%1/typing").arg(channelId), {}, callback);
}

void ApiClient::uploadAttachment(const QString &channelId, const QString &filePath, const QString &content, const ObjectCallback &callback) {
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        callback(false, {}, QStringLiteral("Cannot open file"));
        file->deleteLater();
        return;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(filePath).fileName()));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/octet-stream"));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    if (!content.isEmpty()) {
        QHttpPart contentPart;
        contentPart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"content\""));
        contentPart.setBody(content.toUtf8());
        multiPart->append(contentPart);
    }

    QNetworkRequest request(QUrl(m_baseUrl + QString("/channels/%1/attachments").arg(channelId)));
    request.setRawHeader("Accept", "application/json");
    if (!m_accessToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());
    }
    QNetworkReply *reply = m_network.post(request, multiPart);
    multiPart->setParent(reply);

    QObject::connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            callback(true, QJsonDocument::fromJson(payload).object(), {});
        } else {
            callback(false, {}, ApiClient::parseError(payload));
        }
        reply->deleteLater();
    });
}

void ApiClient::listDmThreads(const ArrayCallback &callback) {
    sendArrayRequest("/dm/threads", callback);
}

void ApiClient::createDmThread(const QStringList &participantIds, const QString &name, const ObjectCallback &callback) {
    QJsonArray participants;
    for (const QString &participant : participantIds) {
        participants.append(participant);
    }

    sendObjectRequest(
        "POST",
        "/dm/threads",
        {
            {"participantIds", participants},
            {"name", name},
        },
        callback);
}

void ApiClient::listDmMessages(const QString &threadId, const QString &cursor, const ObjectCallback &callback) {
    QMap<QString, QString> query;
    if (!cursor.isEmpty()) {
        query.insert("cursor", cursor);
    }

    sendObjectRequest("GET", QString("/dm/threads/%1/messages").arg(threadId), {}, callback, true, query);
}

void ApiClient::sendDmMessage(const QString &threadId, const QString &content, const ObjectCallback &callback) {
    sendObjectRequest(
        "POST",
        QString("/dm/threads/%1/messages").arg(threadId),
        {
            {"content", content},
        },
        callback);
}

void ApiClient::updatePresence(const QString &status, const ObjectCallback &callback) {
    sendObjectRequest("PATCH", "/presence/me", { {"status", status} }, callback);
}

void ApiClient::joinVoice(const QString &guildId, const QString &channelId, bool muted, bool deafened, const ObjectCallback &callback) {
    sendObjectRequest(
        "POST",
        "/voice/join",
        {
            {"guildId", guildId},
            {"channelId", channelId},
            {"muted", muted},
            {"deafened", deafened},
        },
        callback);
}

void ApiClient::issueVoiceSfuToken(const QString &guildId, const QString &channelId, const ObjectCallback &callback) {
    sendObjectRequest(
        "POST",
        "/voice/sfu-token",
        {
            {"guildId", guildId},
            {"channelId", channelId},
        },
        callback);
}

void ApiClient::leaveVoice(const QString &guildId, const ObjectCallback &callback) {
    sendObjectRequest("POST", "/voice/leave", { {"guildId", guildId} }, callback);
}

void ApiClient::updateVoiceState(const QString &guildId, bool muted, bool deafened, const ObjectCallback &callback) {
    sendObjectRequest(
        "PATCH",
        "/voice/state",
        {
            {"guildId", guildId},
            {"muted", muted},
            {"deafened", deafened},
        },
        callback);
}

void ApiClient::listVoiceParticipants(const QString &channelId, const ArrayCallback &callback) {
    sendArrayRequest(QString("/voice/channel/%1").arg(channelId), callback);
}

QNetworkRequest ApiClient::buildRequest(const QString &path, bool withAuth) const {
    QUrl url(m_baseUrl + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    if (withAuth && !m_accessToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_accessToken).toUtf8());
    }

    return request;
}

void ApiClient::sendObjectRequest(
    const QString &method,
    const QString &path,
    const QJsonObject &body,
    const ObjectCallback &callback,
    bool withAuth,
    const QMap<QString, QString> &query,
    int attempt) {

    QNetworkRequest request = buildRequest(path, withAuth);
    QUrl url = request.url();

    if (!query.isEmpty()) {
        QUrlQuery queryItems;
        for (auto it = query.constBegin(); it != query.constEnd(); ++it) {
            queryItems.addQueryItem(it.key(), it.value());
        }
        url.setQuery(queryItems);
        request.setUrl(url);
    }

    QNetworkReply *reply = nullptr;
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    if (method == "GET") {
        reply = m_network.get(request);
    } else if (method == "POST") {
        reply = m_network.post(request, payload);
    } else if (method == "PATCH") {
        reply = m_network.sendCustomRequest(request, "PATCH", payload);
    } else if (method == "PUT") {
        reply = m_network.put(request, payload);
    } else if (method == "DELETE") {
        reply = m_network.deleteResource(request);
    }

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, callback, method, path, body, withAuth, query, attempt]() {
        const QByteArray responsePayload = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            callback(true, QJsonDocument::fromJson(responsePayload).object(), {});
        } else {
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (attempt < kMaxApiRetries && shouldRetryRequest(reply->error(), httpStatus)) {
                const int delayMs = 200 * (attempt + 1);
                QTimer::singleShot(delayMs, this, [this, method, path, body, callback, withAuth, query, attempt]() {
                    sendObjectRequest(method, path, body, callback, withAuth, query, attempt + 1);
                });
                reply->deleteLater();
                return;
            }

            callback(false, {}, ApiClient::parseError(responsePayload));
        }
        reply->deleteLater();
    });
}

void ApiClient::sendArrayRequest(
    const QString &path,
    const ArrayCallback &callback,
    const QMap<QString, QString> &query,
    int attempt) {

    QNetworkRequest request = buildRequest(path, true);
    QUrl url = request.url();

    if (!query.isEmpty()) {
        QUrlQuery queryItems;
        for (auto it = query.constBegin(); it != query.constEnd(); ++it) {
            queryItems.addQueryItem(it.key(), it.value());
        }
        url.setQuery(queryItems);
        request.setUrl(url);
    }

    QNetworkReply *reply = m_network.get(request);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, callback, path, query, attempt]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() == QNetworkReply::NoError) {
            callback(true, QJsonDocument::fromJson(payload).array(), {});
        } else {
            const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (attempt < kMaxApiRetries && shouldRetryRequest(reply->error(), httpStatus)) {
                const int delayMs = 200 * (attempt + 1);
                QTimer::singleShot(delayMs, this, [this, path, callback, query, attempt]() {
                    sendArrayRequest(path, callback, query, attempt + 1);
                });
                reply->deleteLater();
                return;
            }

            callback(false, {}, ApiClient::parseError(payload));
        }
        reply->deleteLater();
    });
}

QString ApiClient::parseError(const QByteArray &payload) {
    auto normalizeError = [](const QString &message) -> QString {
        const QString lowered = message.toLower();
        if (lowered.contains(QStringLiteral("too many requests")) ||
            lowered.contains(QStringLiteral("throttlerexception")) ||
            lowered.contains(QStringLiteral("status code 429")) ||
            lowered == QStringLiteral("429")) {
            return QStringLiteral("Too many requests. Please wait 1-2 seconds and try again.");
        }
        return message;
    };

    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (document.isObject()) {
        const QJsonObject object = document.object();
        if (object.contains("message")) {
            if (object.value("message").isArray()) {
                return normalizeError(object.value("message").toArray().first().toString());
            }
            return normalizeError(object.value("message").toString());
        }
    }

    return normalizeError(QString::fromUtf8(payload));
}
