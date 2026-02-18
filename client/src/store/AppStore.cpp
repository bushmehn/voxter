#include "AppStore.h"

#include <QSet>

AppStore::AppStore(QObject *parent)
    : QObject(parent) {}

bool AppStore::authenticated() const {
    return m_authenticated;
}

QVariantMap AppStore::currentUser() const {
    return m_currentUser;
}

QVariantList AppStore::guilds() const {
    return m_guilds;
}

QVariantList AppStore::guildMembers() const {
    return m_guildMembers;
}

QVariantList AppStore::channels() const {
    return m_channels;
}

QVariantList AppStore::messages() const {
    return m_messages;
}

QVariantList AppStore::dmThreads() const {
    return m_dmThreads;
}

QVariantList AppStore::voiceParticipants() const {
    return m_voiceParticipants;
}

QVariantMap AppStore::voiceParticipantsByChannel() const {
    return m_voiceParticipantsByChannel;
}

QString AppStore::currentGuildId() const {
    return m_currentGuildId;
}

QString AppStore::currentChannelId() const {
    return m_currentChannelId;
}

QString AppStore::currentDmThreadId() const {
    return m_currentDmThreadId;
}

void AppStore::setAuthenticated(bool authenticated) {
    if (m_authenticated == authenticated) {
        return;
    }
    m_authenticated = authenticated;
    emit authenticatedChanged();
}

void AppStore::setCurrentUser(const QVariantMap &user) {
    if (m_currentUser == user) {
        return;
    }
    m_currentUser = user;
    emit currentUserChanged();
}

void AppStore::setGuilds(const QVariantList &guilds) {
    m_guilds = guilds;
    emit guildsChanged();
}

void AppStore::setGuildMembers(const QVariantList &members) {
    m_guildMembers = members;
    emit guildMembersChanged();
}

void AppStore::setChannels(const QVariantList &channels) {
    m_channels = channels;
    emit channelsChanged();
}

void AppStore::setMessages(const QVariantList &messages) {
    m_messages = messages;
    emit messagesChanged();
}

void AppStore::setDmThreads(const QVariantList &threads) {
    m_dmThreads = threads;
    emit dmThreadsChanged();
}

void AppStore::setVoiceParticipants(const QVariantList &participants) {
    if (m_voiceParticipants == participants) {
        return;
    }
    m_voiceParticipants = participants;
    emit voiceParticipantsChanged();
}

void AppStore::setVoiceParticipantsForChannel(const QString &channelId, const QVariantList &participants) {
    if (channelId.isEmpty()) {
        return;
    }

    if (m_voiceParticipantsByChannel.value(channelId).toList() == participants) {
        return;
    }

    m_voiceParticipantsByChannel.insert(channelId, participants);
    emit voiceParticipantsByChannelChanged();
}

QVariantList AppStore::voiceParticipantsForChannel(const QString &channelId) const {
    return m_voiceParticipantsByChannel.value(channelId).toList();
}

void AppStore::setVoiceSpeakingUsers(const QString &channelId, const QStringList &speakingUserIds) {
    if (channelId.isEmpty()) {
        return;
    }

    QVariantList participants = m_voiceParticipantsByChannel.value(channelId).toList();
    if (participants.isEmpty()) {
        return;
    }

    const QSet<QString> speakingSet(speakingUserIds.constBegin(), speakingUserIds.constEnd());
    bool changed = false;

    for (int i = 0; i < participants.size(); ++i) {
        QVariantMap participant = participants.at(i).toMap();
        const QVariantMap participantUser = participant.value("user").toMap();
        const QString participantUserId = participantUser.value("id").toString().isEmpty()
                                              ? participant.value("userId").toString()
                                              : participantUser.value("id").toString();
        if (participantUserId.isEmpty()) {
            continue;
        }

        const bool shouldSpeak = speakingSet.contains(participantUserId);
        if (participant.value("speaking", false).toBool() == shouldSpeak) {
            continue;
        }

        participant.insert("speaking", shouldSpeak);
        participants[i] = participant;
        changed = true;
    }

    if (!changed) {
        return;
    }

    m_voiceParticipantsByChannel.insert(channelId, participants);
    emit voiceParticipantsByChannelChanged();
}

void AppStore::updateVoiceParticipantState(const QString &channelId,
                                           const QString &userId,
                                           const QVariantMap &patch,
                                           bool remove) {
    if (channelId.isEmpty() || userId.isEmpty()) {
        return;
    }

    QVariantList participants = m_voiceParticipantsByChannel.value(channelId).toList();
    bool changed = false;

    for (int i = 0; i < participants.size(); ++i) {
        QVariantMap participant = participants.at(i).toMap();
        const QVariantMap participantUser = participant.value("user").toMap();
        const QString participantUserId = participantUser.value("id").toString().isEmpty()
                                              ? participant.value("userId").toString()
                                              : participantUser.value("id").toString();

        if (participantUserId != userId) {
            continue;
        }

        if (remove) {
            participants.removeAt(i);
            changed = true;
            break;
        }

        const QVariantMap original = participant;
        for (auto it = patch.constBegin(); it != patch.constEnd(); ++it) {
            participant.insert(it.key(), it.value());
        }
        if (participant == original) {
            break;
        }
        participants[i] = participant;
        changed = true;
        break;
    }

    if (!changed && !remove) {
        QVariantMap participant = patch;
        if (!participant.contains("userId")) {
            participant.insert("userId", userId);
        }
        const QVariantMap participantUser = participant.value("user").toMap();
        if (participantUser.value("id").toString().isEmpty() &&
            participant.value("userId").toString().isEmpty()) {
            participant.insert("userId", userId);
        }
        participant.insert("speaking", participant.value("speaking", false));
        participants.append(participant);
        changed = true;
    }

    if (!changed) {
        return;
    }

    m_voiceParticipantsByChannel.insert(channelId, participants);
    emit voiceParticipantsByChannelChanged();
}

void AppStore::removeVoiceParticipantFromAllChannels(const QString &userId, const QString &exceptChannelId) {
    if (userId.isEmpty()) {
        return;
    }

    bool changed = false;

    for (auto it = m_voiceParticipantsByChannel.begin(); it != m_voiceParticipantsByChannel.end(); ++it) {
        const QString channelId = it.key();
        if (!exceptChannelId.isEmpty() && channelId == exceptChannelId) {
            continue;
        }

        QVariantList participants = it.value().toList();
        bool channelChanged = false;

        for (int i = participants.size() - 1; i >= 0; --i) {
            const QVariantMap participant = participants.at(i).toMap();
            const QVariantMap participantUser = participant.value("user").toMap();
            const QString participantUserId = participantUser.value("id").toString().isEmpty()
                                                  ? participant.value("userId").toString()
                                                  : participantUser.value("id").toString();
            if (participantUserId != userId) {
                continue;
            }

            participants.removeAt(i);
            channelChanged = true;
        }

        if (channelChanged) {
            it.value() = participants;
            changed = true;
        }
    }

    if (changed) {
        emit voiceParticipantsByChannelChanged();
    }
}

void AppStore::setCurrentGuildId(const QString &guildId) {
    if (m_currentGuildId == guildId) {
        return;
    }
    m_currentGuildId = guildId;
    emit currentGuildIdChanged();
}

void AppStore::setCurrentChannelId(const QString &channelId) {
    if (m_currentChannelId == channelId) {
        return;
    }
    m_currentChannelId = channelId;
    emit currentChannelIdChanged();
}

void AppStore::setCurrentDmThreadId(const QString &threadId) {
    if (m_currentDmThreadId == threadId) {
        return;
    }
    m_currentDmThreadId = threadId;
    emit currentDmThreadIdChanged();
}

void AppStore::upsertMessage(const QVariantMap &message) {
    const QString id = message.value("id").toString();
    if (id.isEmpty()) {
        return;
    }

    for (int i = 0; i < m_messages.size(); ++i) {
        const QVariantMap existing = m_messages.at(i).toMap();
        if (existing.value("id").toString() == id) {
            m_messages[i] = message;
            emit messagesChanged();
            return;
        }
    }

    m_messages.append(message);
    emit messagesChanged();
}

void AppStore::removeMessage(const QString &messageId) {
    for (int i = 0; i < m_messages.size(); ++i) {
        const QVariantMap existing = m_messages.at(i).toMap();
        if (existing.value("id").toString() == messageId) {
            m_messages.removeAt(i);
            emit messagesChanged();
            return;
        }
    }
}

void AppStore::clearSession() {
    m_authenticated = false;
    m_currentUser.clear();
    m_guilds.clear();
    m_guildMembers.clear();
    m_channels.clear();
    m_messages.clear();
    m_dmThreads.clear();
    m_voiceParticipants.clear();
    m_voiceParticipantsByChannel.clear();
    m_currentGuildId.clear();
    m_currentChannelId.clear();
    m_currentDmThreadId.clear();

    emit authenticatedChanged();
    emit currentUserChanged();
    emit guildsChanged();
    emit guildMembersChanged();
    emit channelsChanged();
    emit messagesChanged();
    emit dmThreadsChanged();
    emit voiceParticipantsChanged();
    emit voiceParticipantsByChannelChanged();
    emit currentGuildIdChanged();
    emit currentChannelIdChanged();
    emit currentDmThreadIdChanged();
}
