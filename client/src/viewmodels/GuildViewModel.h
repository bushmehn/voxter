#pragma once

#include <QObject>
#include <QJsonObject>

class ApiClient;
class GatewayClient;
class AppStore;

class GuildViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString lastInviteCode READ lastInviteCode NOTIFY lastInviteCodeChanged)

public:
    explicit GuildViewModel(QObject *parent = nullptr);

    void setup(ApiClient *api, GatewayClient *gateway, AppStore *store);

    bool busy() const;
    QString error() const;
    QString lastInviteCode() const;

    Q_INVOKABLE void loadGuilds();
    Q_INVOKABLE void selectGuild(const QString &guildId);
    Q_INVOKABLE void createGuild(const QString &name);
    Q_INVOKABLE void createChannel(const QString &name, const QString &type);
    Q_INVOKABLE void createInvite();
    Q_INVOKABLE void joinInvite(const QString &code);
    Q_INVOKABLE void loadDmThreads();
    Q_INVOKABLE void createDmThread(const QStringList &participantIds, const QString &name);
    void handleGatewayEvent(const QString &eventName, const QJsonObject &payload);

signals:
    void busyChanged();
    void errorChanged();
    void lastInviteCodeChanged();

private:
    void setBusy(bool busy);
    void setError(const QString &error);
    void loadChannels(const QString &guildId);
    void loadGuildMembers();

    ApiClient *m_api{nullptr};
    GatewayClient *m_gateway{nullptr};
    AppStore *m_store{nullptr};
    bool m_busy{false};
    QString m_error;
    QString m_lastInviteCode;
};
