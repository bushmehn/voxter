#include "GatewayClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <utility>

GatewayClient::GatewayClient(QObject *parent)
    : QObject(parent) {

    m_reconnectTimer.setSingleShot(true);
    QObject::connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_manualDisconnect || m_accessToken.isEmpty()) {
            return;
        }

        setConnectionState(QStringLiteral("reconnecting"));
        m_socket.open(QUrl(makeGatewayUrl(m_accessToken)));
    });

    QObject::connect(&m_socket, &QWebSocket::connected, this, [this]() {
        m_reconnectTimer.stop();
        setReconnectAttempt(0);
        setConnectionState(QStringLiteral("connected"));

        if (!m_connected) {
            m_connected = true;
            emit connectedChanged();
        }

        sendSubscriptions();
    });

    QObject::connect(&m_socket, &QWebSocket::disconnected, this, [this]() {
        if (m_connected) {
            m_connected = false;
            emit connectedChanged();
        }

        if (m_manualDisconnect || m_accessToken.isEmpty()) {
            setConnectionState(QStringLiteral("disconnected"));
            return;
        }

        scheduleReconnect();
    });

    QObject::connect(&m_socket, &QWebSocket::textMessageReceived, this, [this](const QString &text) {
        const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8());
        if (!document.isObject()) {
            return;
        }

        const QJsonObject object = document.object();
        const QString eventName = object.value("t").toString();
        const QJsonObject payload = object.value("d").toObject();
        emit eventReceived(eventName, payload);
    });

    QObject::connect(&m_socket,
                     &QWebSocket::errorOccurred,
                     this,
                     [this](QAbstractSocket::SocketError) {
                         emit socketError(m_socket.errorString());
                         if (!m_connected && !m_manualDisconnect && !m_accessToken.isEmpty() && !m_reconnectTimer.isActive()) {
                             scheduleReconnect();
                         }
                     });
}

bool GatewayClient::connected() const {
    return m_connected;
}

QString GatewayClient::connectionState() const {
    return m_connectionState;
}

int GatewayClient::reconnectAttempt() const {
    return m_reconnectAttempt;
}

void GatewayClient::setApiBaseUrl(const QString &baseUrl) {
    m_apiBaseUrl = baseUrl;
}

void GatewayClient::connectToGateway(const QString &accessToken) {
    if (accessToken.isEmpty()) {
        return;
    }

    if (m_socket.state() == QAbstractSocket::ConnectedState && m_accessToken == accessToken) {
        return;
    }

    m_accessToken = accessToken;
    m_manualDisconnect = false;
    m_reconnectTimer.stop();
    setReconnectAttempt(0);
    setConnectionState(QStringLiteral("connecting"));
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.abort();
    }
    m_socket.open(QUrl(makeGatewayUrl(m_accessToken)));
}

void GatewayClient::disconnectFromGateway() {
    m_manualDisconnect = true;
    m_reconnectTimer.stop();
    m_accessToken.clear();
    m_desiredChannelIds.clear();
    m_desiredDmThreadIds.clear();
    setReconnectAttempt(0);
    setConnectionState(QStringLiteral("disconnected"));
    m_socket.close();
}

void GatewayClient::sendEvent(const QString &eventName, const QJsonObject &data) {
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }

    const QJsonObject payload {
        {"event", eventName},
        {"data", data},
    };

    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
}

void GatewayClient::subscribeChannels(const QStringList &channelIds) {
    m_desiredChannelIds = channelIds;

    QJsonArray ids;
    for (const QString &id : channelIds) {
        ids.append(id);
    }

    sendEvent("subscribe_channels", { {"channelIds", ids} });
}

void GatewayClient::subscribeDmThreads(const QStringList &threadIds) {
    m_desiredDmThreadIds = threadIds;

    QJsonArray ids;
    for (const QString &id : threadIds) {
        ids.append(id);
    }

    sendEvent("subscribe_dm_threads", { {"threadIds", ids} });
}

QString GatewayClient::makeGatewayUrl(const QString &accessToken) const {
    QUrl url(m_apiBaseUrl);
    if (url.scheme() == "https") {
        url.setScheme("wss");
    } else {
        url.setScheme("ws");
    }
    url.setPath("/gateway");
    url.setQuery(QString("token=%1").arg(accessToken));
    return url.toString();
}

void GatewayClient::setConnectionState(const QString &state) {
    if (m_connectionState == state) {
        return;
    }

    m_connectionState = state;
    emit connectionStateChanged();
}

void GatewayClient::setReconnectAttempt(int attempt) {
    if (m_reconnectAttempt == attempt) {
        return;
    }

    m_reconnectAttempt = attempt;
    emit reconnectAttemptChanged();
}

void GatewayClient::scheduleReconnect() {
    const int nextAttempt = m_reconnectAttempt + 1;
    setReconnectAttempt(nextAttempt);
    setConnectionState(QStringLiteral("reconnecting"));

    const int delayMs = qMin(30000, 500 * (1 << qMin(6, nextAttempt - 1)));
    m_reconnectTimer.start(delayMs);
}

void GatewayClient::sendSubscriptions() {
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }

    if (!m_desiredChannelIds.isEmpty()) {
        QJsonArray channelIds;
        for (const QString &id : std::as_const(m_desiredChannelIds)) {
            channelIds.append(id);
        }
        sendEvent(QStringLiteral("subscribe_channels"), {{"channelIds", channelIds}});
    }

    if (!m_desiredDmThreadIds.isEmpty()) {
        QJsonArray threadIds;
        for (const QString &id : std::as_const(m_desiredDmThreadIds)) {
            threadIds.append(id);
        }
        sendEvent(QStringLiteral("subscribe_dm_threads"), {{"threadIds", threadIds}});
    }
}
