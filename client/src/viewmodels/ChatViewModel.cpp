#include "ChatViewModel.h"

#include "services/ApiClient.h"
#include "store/AppStore.h"

#include <QFileInfo>
#include <QUrl>

ChatViewModel::ChatViewModel(QObject *parent)
    : QObject(parent) {}

void ChatViewModel::setup(ApiClient *api, AppStore *store) {
    m_api = api;
    m_store = store;
}

bool ChatViewModel::busy() const {
    return m_busy;
}

QString ChatViewModel::error() const {
    return m_error;
}

void ChatViewModel::loadChannelMessages(bool reset) {
    if (!m_api || !m_store || m_store->currentChannelId().isEmpty()) {
        return;
    }

    setBusy(true);
    const QString cursor = reset ? QString() : m_channelCursor;
    if (reset) {
        m_channelCursor.clear();
    }

    m_api->listMessages(m_store->currentChannelId(), cursor, {}, [this, reset](bool ok, const QJsonObject &response, const QString &error) {
        setBusy(false);
        if (!ok) {
            setError(error);
            return;
        }

        const QJsonArray items = response.value("items").toArray();
        const QVariantList timeline = toChronological(items);

        if (reset) {
            m_store->setMessages(timeline);
        } else {
            QVariantList current = m_store->messages();
            for (const QVariant &item : timeline) {
                current.prepend(item);
            }
            m_store->setMessages(current);
        }

        m_channelCursor = response.value("nextCursor").toString();
        setError({});
    });
}

void ChatViewModel::sendMessage(const QString &content) {
    if (!m_api || !m_store || content.trimmed().isEmpty() || m_store->currentChannelId().isEmpty()) {
        return;
    }

    m_api->sendMessage(m_store->currentChannelId(), content.trimmed(), [this](bool ok, const QJsonObject &response, const QString &error) {
        if (!ok) {
            setError(error);
            return;
        }

        m_store->upsertMessage(response.toVariantMap());
        setError({});
    });
}

void ChatViewModel::editMessage(const QString &messageId, const QString &content) {
    if (!m_api || !m_store || messageId.isEmpty() || content.trimmed().isEmpty() || m_store->currentChannelId().isEmpty()) {
        return;
    }

    m_api->editMessage(
        m_store->currentChannelId(),
        messageId,
        content,
        [this](bool ok, const QJsonObject &response, const QString &error) {
            if (!ok) {
                setError(error);
                return;
            }
            m_store->upsertMessage(response.toVariantMap());
            setError({});
        });
}

void ChatViewModel::deleteMessage(const QString &messageId) {
    if (!m_api || !m_store || messageId.isEmpty() || m_store->currentChannelId().isEmpty()) {
        return;
    }

    m_api->deleteMessage(m_store->currentChannelId(), messageId, [this, messageId](bool ok, const QJsonObject &, const QString &error) {
        if (!ok) {
            setError(error);
            return;
        }

        m_store->removeMessage(messageId);
        setError({});
    });
}

void ChatViewModel::reactToMessage(const QString &messageId, const QString &emoji) {
    if (!m_api || !m_store || messageId.isEmpty() || emoji.isEmpty() || m_store->currentChannelId().isEmpty()) {
        return;
    }

    m_api->toggleReaction(
        m_store->currentChannelId(),
        messageId,
        emoji,
        [this](bool ok, const QJsonObject &, const QString &error) {
            if (!ok) {
                setError(error);
                return;
            }
            setError({});
        });
}

void ChatViewModel::sendTyping() {
    if (!m_api || !m_store || m_store->currentChannelId().isEmpty()) {
        return;
    }

    m_api->sendTyping(m_store->currentChannelId(), [this](bool ok, const QJsonObject &, const QString &error) {
        if (!ok) {
            setError(error);
        }
    });
}

void ChatViewModel::uploadAttachment(const QString &filePath, const QString &content) {
    if (!m_api || !m_store || m_store->currentChannelId().isEmpty()) {
        return;
    }

    QString normalizedPath = filePath;
    if (normalizedPath.startsWith("file:///")) {
        normalizedPath = QUrl(filePath).toLocalFile();
    }

    if (!QFileInfo::exists(normalizedPath)) {
        setError(QStringLiteral("File not found"));
        return;
    }

    m_api->uploadAttachment(
        m_store->currentChannelId(),
        normalizedPath,
        content,
        [this](bool ok, const QJsonObject &response, const QString &error) {
            if (!ok) {
                setError(error);
                return;
            }
            m_store->upsertMessage(response.toVariantMap());
            setError({});
        });
}

void ChatViewModel::selectDmThread(const QString &threadId) {
    if (!m_store || threadId.isEmpty()) {
        return;
    }

    m_store->setCurrentDmThreadId(threadId);
    m_store->setCurrentChannelId({});
    loadDmMessages(true);
}

void ChatViewModel::loadDmMessages(bool reset) {
    if (!m_api || !m_store || m_store->currentDmThreadId().isEmpty()) {
        return;
    }

    setBusy(true);
    const QString cursor = reset ? QString() : m_dmCursor;
    if (reset) {
        m_dmCursor.clear();
    }

    m_api->listDmMessages(m_store->currentDmThreadId(), cursor, [this, reset](bool ok, const QJsonObject &response, const QString &error) {
        setBusy(false);
        if (!ok) {
            setError(error);
            return;
        }

        const QVariantList timeline = toChronological(response.value("items").toArray());
        if (reset) {
            m_store->setMessages(timeline);
        } else {
            QVariantList current = m_store->messages();
            for (const QVariant &item : timeline) {
                current.prepend(item);
            }
            m_store->setMessages(current);
        }

        m_dmCursor = response.value("nextCursor").toString();
        setError({});
    });
}

void ChatViewModel::sendDmMessage(const QString &content) {
    if (!m_api || !m_store || m_store->currentDmThreadId().isEmpty() || content.trimmed().isEmpty()) {
        return;
    }

    m_api->sendDmMessage(m_store->currentDmThreadId(), content.trimmed(), [this](bool ok, const QJsonObject &response, const QString &error) {
        if (!ok) {
            setError(error);
            return;
        }

        m_store->upsertMessage(response.toVariantMap());
        setError({});
    });
}

void ChatViewModel::handleGatewayEvent(const QString &eventName, const QJsonObject &payload) {
    if (!m_store) {
        return;
    }

    if (eventName == "message_create") {
        if (payload.value("channelId").toString() == m_store->currentChannelId()) {
            m_store->upsertMessage(payload.toVariantMap());
        }
        return;
    }

    if (eventName == "message_update") {
        if (payload.value("channelId").toString() == m_store->currentChannelId()) {
            m_store->upsertMessage(payload.toVariantMap());
        }
        return;
    }

    if (eventName == "message_delete") {
        if (payload.value("channelId").toString() == m_store->currentChannelId()) {
            m_store->removeMessage(payload.value("messageId").toString());
        }
        return;
    }

    if (eventName == "dm_message_create") {
        if (payload.value("dmThreadId").toString() == m_store->currentDmThreadId()) {
            m_store->upsertMessage(payload.toVariantMap());
        }
        return;
    }

    if (eventName == "message_reaction_add" || eventName == "message_reaction_remove") {
        const QString targetMessageId = payload.value("messageId").toString();
        QVariantList messages = m_store->messages();
        for (int i = 0; i < messages.size(); ++i) {
            QVariantMap message = messages.at(i).toMap();
            if (message.value("id").toString() != targetMessageId) {
                continue;
            }

            QVariantList reactions = message.value("reactions").toList();
            const QString userId = payload.value("userId").toString();
            const QString emoji = payload.value("emoji").toString();

            if (eventName == "message_reaction_add") {
                QVariantMap reaction;
                reaction.insert("userId", userId);
                reaction.insert("emoji", emoji);
                reactions.append(reaction);
            } else {
                for (int j = reactions.size() - 1; j >= 0; --j) {
                    const QVariantMap existing = reactions.at(j).toMap();
                    if (existing.value("userId").toString() == userId &&
                        existing.value("emoji").toString() == emoji) {
                        reactions.removeAt(j);
                        break;
                    }
                }
            }

            message.insert("reactions", reactions);
            messages[i] = message;
            m_store->setMessages(messages);
            break;
        }
    }
}

void ChatViewModel::setBusy(bool busy) {
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void ChatViewModel::setError(const QString &error) {
    if (m_error == error) {
        return;
    }
    m_error = error;
    emit errorChanged();
}

QVariantList ChatViewModel::toChronological(const QJsonArray &items) {
    QVariantList output;
    output.reserve(items.size());

    for (int i = items.size() - 1; i >= 0; --i) {
        output.append(items.at(i).toObject().toVariantMap());
    }

    return output;
}
