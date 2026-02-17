#include "GuildViewModel.h"

#include "services/ApiClient.h"
#include "services/GatewayClient.h"
#include "store/AppStore.h"

GuildViewModel::GuildViewModel(QObject *parent)
    : QObject(parent) {}

void GuildViewModel::setup(ApiClient *api, GatewayClient *gateway, AppStore *store) {
    m_api = api;
    m_gateway = gateway;
    m_store = store;
}

bool GuildViewModel::busy() const {
    return m_busy;
}

QString GuildViewModel::error() const {
    return m_error;
}

QString GuildViewModel::lastInviteCode() const {
    return m_lastInviteCode;
}

void GuildViewModel::loadGuilds() {
    if (!m_api || !m_store) {
        return;
    }

    setBusy(true);
    m_api->listGuilds([this](bool ok, const QJsonArray &guilds, const QString &error) {
        setBusy(false);

        if (!ok) {
            setError(error);
            return;
        }

        m_store->setGuilds(guilds.toVariantList());
        if (m_store->currentGuildId().isEmpty() && !guilds.isEmpty()) {
            const QString guildId = guilds.first().toObject().value("id").toString();
            selectGuild(guildId);
        } else if (!m_store->currentGuildId().isEmpty()) {
            loadChannels(m_store->currentGuildId());
            loadGuildMembers();
        }

        setError({});
    });
}

void GuildViewModel::selectGuild(const QString &guildId) {
    if (!m_store || guildId.isEmpty()) {
        return;
    }

    m_store->setCurrentGuildId(guildId);
    loadChannels(guildId);
    loadGuildMembers();
}

void GuildViewModel::createGuild(const QString &name) {
    if (!m_api || !m_store || name.trimmed().isEmpty()) {
        return;
    }

    setBusy(true);
    m_api->createGuild(name.trimmed(), [this](bool ok, const QJsonObject &, const QString &error) {
        setBusy(false);
        if (!ok) {
            setError(error);
            return;
        }
        loadGuilds();
        setError({});
    });
}

void GuildViewModel::createChannel(const QString &name, const QString &type) {
    if (!m_api || !m_store || m_store->currentGuildId().isEmpty() || name.trimmed().isEmpty()) {
        return;
    }

    setBusy(true);
    m_api->createChannel(
        m_store->currentGuildId(),
        name.trimmed(),
        type,
        [this](bool ok, const QJsonObject &, const QString &error) {
            setBusy(false);
            if (!ok) {
                setError(error);
                return;
            }
            loadChannels(m_store->currentGuildId());
            setError({});
        });
}

void GuildViewModel::createInvite() {
    if (!m_api || !m_store || m_store->currentGuildId().isEmpty()) {
        return;
    }

    m_api->createInvite(m_store->currentGuildId(), [this](bool ok, const QJsonObject &response, const QString &error) {
        if (!ok) {
            setError(error);
            return;
        }

        m_lastInviteCode = response.value("code").toString();
        emit lastInviteCodeChanged();
        setError({});
    });
}

void GuildViewModel::joinInvite(const QString &code) {
    if (!m_api || code.trimmed().isEmpty()) {
        return;
    }

    setBusy(true);
    m_api->joinInvite(code.trimmed(), [this](bool ok, const QJsonObject &, const QString &error) {
        setBusy(false);
        if (!ok) {
            setError(error);
            return;
        }

        loadGuilds();
        setError({});
    });
}

void GuildViewModel::loadDmThreads() {
    if (!m_api || !m_store || !m_gateway) {
        return;
    }

    m_api->listDmThreads([this](bool ok, const QJsonArray &threads, const QString &error) {
        if (!ok) {
            setError(error);
            return;
        }

        m_store->setDmThreads(threads.toVariantList());
        QStringList ids;
        for (const QJsonValue &value : threads) {
            ids.append(value.toObject().value("id").toString());
        }
        m_gateway->subscribeDmThreads(ids);
        setError({});
    });
}

void GuildViewModel::createDmThread(const QStringList &participantIds, const QString &name) {
    if (!m_api || participantIds.isEmpty()) {
        return;
    }

    setBusy(true);
    m_api->createDmThread(participantIds, name, [this](bool ok, const QJsonObject &, const QString &error) {
        setBusy(false);
        if (!ok) {
            setError(error);
            return;
        }

        loadDmThreads();
        setError({});
    });
}

void GuildViewModel::handleGatewayEvent(const QString &eventName, const QJsonObject &payload) {
    if (!m_store) {
        return;
    }

    if (eventName == "guild_member_join" && payload.value("guildId").toString() == m_store->currentGuildId()) {
        loadGuildMembers();
        return;
    }

    if (eventName != "presence_update") {
        return;
    }

    const QString userId = payload.value("userId").toString();
    if (userId.isEmpty()) {
        return;
    }

    QVariantList members = m_store->guildMembers();
    bool changed = false;

    for (int i = 0; i < members.size(); ++i) {
        QVariantMap member = members.at(i).toMap();
        if (member.value("id").toString() != userId) {
            continue;
        }

        member.insert("presence", payload.value("status").toString());
        members[i] = member;
        changed = true;
        break;
    }

    if (changed) {
        m_store->setGuildMembers(members);
    }
}

void GuildViewModel::setBusy(bool busy) {
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void GuildViewModel::setError(const QString &error) {
    if (m_error == error) {
        return;
    }
    m_error = error;
    emit errorChanged();
}

void GuildViewModel::loadChannels(const QString &guildId) {
    if (!m_api || !m_store || !m_gateway || guildId.isEmpty()) {
        return;
    }

    m_api->listChannels(guildId, [this](bool ok, const QJsonArray &channels, const QString &error) {
        if (!ok) {
            setError(error);
            return;
        }

        m_store->setChannels(channels.toVariantList());
        QStringList channelIds;
        QString firstTextChannel;

        for (const QJsonValue &value : channels) {
            const QJsonObject channel = value.toObject();
            const QString channelId = channel.value("id").toString();
            channelIds.append(channelId);
            if (firstTextChannel.isEmpty() && channel.value("type").toString() == "TEXT") {
                firstTextChannel = channelId;
            }

            if (channel.value("type").toString() == "VOICE") {
                m_api->listVoiceParticipants(channelId, [this, channelId](bool previewOk, const QJsonArray &participants, const QString &) {
                    if (!previewOk || !m_store) {
                        return;
                    }
                    m_store->setVoiceParticipantsForChannel(channelId, participants.toVariantList());
                });
            }
        }

        m_gateway->subscribeChannels(channelIds);

        if (m_store->currentChannelId().isEmpty() && !firstTextChannel.isEmpty()) {
            m_store->setCurrentChannelId(firstTextChannel);
        }

        setError({});
    });
}

void GuildViewModel::loadGuildMembers() {
    if (!m_api || !m_store || m_store->currentGuildId().isEmpty()) {
        return;
    }

    m_api->listGuildPresence(m_store->currentGuildId(), [this](bool ok, const QJsonArray &members, const QString &error) {
        if (!ok) {
            setError(error);
            return;
        }

        m_store->setGuildMembers(members.toVariantList());
        setError({});
    });
}
