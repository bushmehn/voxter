#pragma once

#include <QObject>

class ApiClient;
class AppStore;

class ProfileViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit ProfileViewModel(QObject *parent = nullptr);

    void setup(ApiClient *api, AppStore *store);

    bool busy() const;
    QString error() const;

    Q_INVOKABLE void saveProfile(const QString &displayName);

signals:
    void busyChanged();
    void errorChanged();
    void saved();

private:
    void setBusy(bool busy);
    void setError(const QString &error);

    ApiClient *m_api{nullptr};
    AppStore *m_store{nullptr};
    bool m_busy{false};
    QString m_error;
};
