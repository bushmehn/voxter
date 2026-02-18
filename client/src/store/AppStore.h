#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class AppStore final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(QVariantMap currentUser READ currentUser NOTIFY currentUserChanged)
    Q_PROPERTY(QVariantList guilds READ guilds NOTIFY guildsChanged)
    Q_PROPERTY(QVariantList guildMembers READ guildMembers NOTIFY guildMembersChanged)
    Q_PROPERTY(QVariantList channels READ channels NOTIFY channelsChanged)
    Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QVariantList dmThreads READ dmThreads NOTIFY dmThreadsChanged)
    Q_PROPERTY(QVariantList voiceParticipants READ voiceParticipants NOTIFY voiceParticipantsChanged)
    Q_PROPERTY(QVariantMap voiceParticipantsByChannel READ voiceParticipantsByChannel NOTIFY voiceParticipantsByChannelChanged)
    Q_PROPERTY(QString currentGuildId READ currentGuildId WRITE setCurrentGuildId NOTIFY currentGuildIdChanged)
    Q_PROPERTY(QString currentChannelId READ currentChannelId WRITE setCurrentChannelId NOTIFY currentChannelIdChanged)
    Q_PROPERTY(QString currentDmThreadId READ currentDmThreadId WRITE setCurrentDmThreadId NOTIFY currentDmThreadIdChanged)

public:
    explicit AppStore(QObject *parent = nullptr);

    bool authenticated() const;
    QVariantMap currentUser() const;
    QVariantList guilds() const;
    QVariantList guildMembers() const;
    QVariantList channels() const;
    QVariantList messages() const;
    QVariantList dmThreads() const;
    QVariantList voiceParticipants() const;
    QVariantMap voiceParticipantsByChannel() const;
    QString currentGuildId() const;
    QString currentChannelId() const;
    QString currentDmThreadId() const;

    Q_INVOKABLE void setAuthenticated(bool authenticated);
    Q_INVOKABLE void setCurrentUser(const QVariantMap &user);
    Q_INVOKABLE void setGuilds(const QVariantList &guilds);
    Q_INVOKABLE void setGuildMembers(const QVariantList &members);
    Q_INVOKABLE void setChannels(const QVariantList &channels);
    Q_INVOKABLE void setMessages(const QVariantList &messages);
    Q_INVOKABLE void setDmThreads(const QVariantList &threads);
    Q_INVOKABLE void setVoiceParticipants(const QVariantList &participants);
    Q_INVOKABLE void setVoiceParticipantsForChannel(const QString &channelId, const QVariantList &participants);
    Q_INVOKABLE QVariantList voiceParticipantsForChannel(const QString &channelId) const;
    Q_INVOKABLE void setVoiceSpeakingUsers(const QString &channelId, const QStringList &speakingUserIds);
    Q_INVOKABLE void updateVoiceParticipantState(const QString &channelId,
                                                 const QString &userId,
                                                 const QVariantMap &patch,
                                                 bool remove = false);
    Q_INVOKABLE void removeVoiceParticipantFromAllChannels(const QString &userId,
                                                           const QString &exceptChannelId = QString());
    Q_INVOKABLE void setCurrentGuildId(const QString &guildId);
    Q_INVOKABLE void setCurrentChannelId(const QString &channelId);
    Q_INVOKABLE void setCurrentDmThreadId(const QString &threadId);
    Q_INVOKABLE void upsertMessage(const QVariantMap &message);
    Q_INVOKABLE void removeMessage(const QString &messageId);
    Q_INVOKABLE void clearSession();

signals:
    void authenticatedChanged();
    void currentUserChanged();
    void guildsChanged();
    void guildMembersChanged();
    void channelsChanged();
    void messagesChanged();
    void dmThreadsChanged();
    void voiceParticipantsChanged();
    void voiceParticipantsByChannelChanged();
    void currentGuildIdChanged();
    void currentChannelIdChanged();
    void currentDmThreadIdChanged();

private:
    bool m_authenticated{false};
    QVariantMap m_currentUser;
    QVariantList m_guilds;
    QVariantList m_guildMembers;
    QVariantList m_channels;
    QVariantList m_messages;
    QVariantList m_dmThreads;
    QVariantList m_voiceParticipants;
    QVariantMap m_voiceParticipantsByChannel;
    QString m_currentGuildId;
    QString m_currentChannelId;
    QString m_currentDmThreadId;
};
