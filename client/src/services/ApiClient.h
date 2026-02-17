#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QStringList>
#include <functional>

class ApiClient final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)

public:
    using ObjectCallback = std::function<void(bool, const QJsonObject &, const QString &)>;
    using ArrayCallback = std::function<void(bool, const QJsonArray &, const QString &)>;

    explicit ApiClient(QObject *parent = nullptr);

    QString baseUrl() const;
    void setBaseUrl(const QString &baseUrl);

    QString accessToken() const;
    QString refreshToken() const;
    void setAccessToken(const QString &token);
    void setRefreshToken(const QString &token);
    void clearTokens();

    void registerUser(const QString &email, const QString &password, const QString &displayName, const ObjectCallback &callback);
    void login(const QString &email, const QString &password, const ObjectCallback &callback);
    void refresh(const QString &refreshToken, const ObjectCallback &callback);
    void me(const ObjectCallback &callback);
    void updateProfile(const QString &displayName, const ObjectCallback &callback);

    void listGuilds(const ArrayCallback &callback);
    void listGuildPresence(const QString &guildId, const ArrayCallback &callback);
    void createGuild(const QString &name, const ObjectCallback &callback);
    void listChannels(const QString &guildId, const ArrayCallback &callback);
    void createChannel(const QString &guildId, const QString &name, const QString &type, const ObjectCallback &callback);
    void createInvite(const QString &guildId, const ObjectCallback &callback);
    void joinInvite(const QString &code, const ObjectCallback &callback);

    void listMessages(const QString &channelId, const QString &cursor, const QString &search, const ObjectCallback &callback);
    void sendMessage(const QString &channelId, const QString &content, const ObjectCallback &callback);
    void editMessage(const QString &channelId, const QString &messageId, const QString &content, const ObjectCallback &callback);
    void deleteMessage(const QString &channelId, const QString &messageId, const ObjectCallback &callback);
    void toggleReaction(const QString &channelId, const QString &messageId, const QString &emoji, const ObjectCallback &callback);
    void sendTyping(const QString &channelId, const ObjectCallback &callback);
    void uploadAttachment(const QString &channelId, const QString &filePath, const QString &content, const ObjectCallback &callback);

    void listDmThreads(const ArrayCallback &callback);
    void createDmThread(const QStringList &participantIds, const QString &name, const ObjectCallback &callback);
    void listDmMessages(const QString &threadId, const QString &cursor, const ObjectCallback &callback);
    void sendDmMessage(const QString &threadId, const QString &content, const ObjectCallback &callback);

    void updatePresence(const QString &status, const ObjectCallback &callback);

    void joinVoice(const QString &guildId, const QString &channelId, bool muted, bool deafened, const ObjectCallback &callback);
    void issueVoiceSfuToken(const QString &guildId, const QString &channelId, const ObjectCallback &callback);
    void leaveVoice(const QString &guildId, const ObjectCallback &callback);
    void updateVoiceState(const QString &guildId, bool muted, bool deafened, const ObjectCallback &callback);
    void listVoiceParticipants(const QString &channelId, const ArrayCallback &callback);

signals:
    void baseUrlChanged();

private:
    QNetworkRequest buildRequest(const QString &path, bool withAuth = true) const;
    void sendObjectRequest(
        const QString &method,
        const QString &path,
        const QJsonObject &body,
        const ObjectCallback &callback,
        bool withAuth = true,
        const QMap<QString, QString> &query = {},
        int attempt = 0);

    void sendArrayRequest(
        const QString &path,
        const ArrayCallback &callback,
        const QMap<QString, QString> &query = {},
        int attempt = 0);

    static QString parseError(const QByteArray &payload);

    QNetworkAccessManager m_network;
    QString m_baseUrl{"http://localhost:4000"};
    QString m_accessToken;
    QString m_refreshToken;
};
