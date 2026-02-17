#pragma once

#include <QObject>
#include <QJsonObject>
#include <QVariantMap>

class ApiClient;
class GatewayClient;
class AppStore;
class SecureStorage;

class AuthViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit AuthViewModel(QObject *parent = nullptr);

    void setup(ApiClient *api,
               GatewayClient *gateway,
               AppStore *store,
               SecureStorage *storage);

    bool busy() const;
    QString error() const;

    Q_INVOKABLE void bootstrapSession();
    Q_INVOKABLE void login(const QString &email, const QString &password);
    Q_INVOKABLE void registerUser(const QString &email, const QString &password, const QString &displayName);
    Q_INVOKABLE void logout();

signals:
    void busyChanged();
    void errorChanged();
    void sessionReady();

private:
    void setBusy(bool busy);
    void setError(const QString &error);
    void applyAuthResponse(const QJsonObject &response);

    ApiClient *m_api{nullptr};
    GatewayClient *m_gateway{nullptr};
    AppStore *m_store{nullptr};
    SecureStorage *m_storage{nullptr};
    bool m_busy{false};
    QString m_error;
};
