#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QWebSocket>
#include <QJsonObject>

class GatewayClient final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(int reconnectAttempt READ reconnectAttempt NOTIFY reconnectAttemptChanged)

public:
    explicit GatewayClient(QObject *parent = nullptr);

    bool connected() const;
    QString connectionState() const;
    int reconnectAttempt() const;

    void setApiBaseUrl(const QString &baseUrl);

    Q_INVOKABLE void connectToGateway(const QString &accessToken);
    Q_INVOKABLE void disconnectFromGateway();
    Q_INVOKABLE void sendEvent(const QString &eventName, const QJsonObject &data);
    Q_INVOKABLE void subscribeChannels(const QStringList &channelIds);
    Q_INVOKABLE void subscribeDmThreads(const QStringList &threadIds);

signals:
    void connectedChanged();
    void connectionStateChanged();
    void reconnectAttemptChanged();
    void eventReceived(const QString &eventName, const QJsonObject &payload);
    void socketError(const QString &message);

private:
    void setConnectionState(const QString &state);
    void setReconnectAttempt(int attempt);
    void scheduleReconnect();
    void sendSubscriptions();
    QString makeGatewayUrl(const QString &accessToken) const;

    QWebSocket m_socket;
    QTimer m_reconnectTimer;
    QString m_apiBaseUrl{"http://localhost:4000"};
    QString m_accessToken;
    QStringList m_desiredChannelIds;
    QStringList m_desiredDmThreadIds;
    bool m_connected{false};
    bool m_manualDisconnect{false};
    QString m_connectionState{"disconnected"};
    int m_reconnectAttempt{0};
};
