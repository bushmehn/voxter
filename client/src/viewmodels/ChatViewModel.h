#pragma once

#include <QObject>
#include <QJsonObject>

class ApiClient;
class AppStore;

class ChatViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit ChatViewModel(QObject *parent = nullptr);

    void setup(ApiClient *api, AppStore *store);

    bool busy() const;
    QString error() const;

    Q_INVOKABLE void loadChannelMessages(bool reset = true);
    Q_INVOKABLE void sendMessage(const QString &content);
    Q_INVOKABLE void editMessage(const QString &messageId, const QString &content);
    Q_INVOKABLE void deleteMessage(const QString &messageId);
    Q_INVOKABLE void reactToMessage(const QString &messageId, const QString &emoji);
    Q_INVOKABLE void sendTyping();
    Q_INVOKABLE void uploadAttachment(const QString &filePath, const QString &content);

    Q_INVOKABLE void selectDmThread(const QString &threadId);
    Q_INVOKABLE void loadDmMessages(bool reset = true);
    Q_INVOKABLE void sendDmMessage(const QString &content);

    void handleGatewayEvent(const QString &eventName, const QJsonObject &payload);

signals:
    void busyChanged();
    void errorChanged();

private:
    void setBusy(bool busy);
    void setError(const QString &error);
    static QVariantList toChronological(const QJsonArray &items);

    ApiClient *m_api{nullptr};
    AppStore *m_store{nullptr};
    QString m_channelCursor;
    QString m_dmCursor;
    bool m_busy{false};
    QString m_error;
};
