#include "AuthViewModel.h"

#include "services/ApiClient.h"
#include "services/GatewayClient.h"
#include "services/SecureStorage.h"
#include "store/AppStore.h"

AuthViewModel::AuthViewModel(QObject *parent)
    : QObject(parent) {}

void AuthViewModel::setup(ApiClient *api,
                          GatewayClient *gateway,
                          AppStore *store,
                          SecureStorage *storage) {
    m_api = api;
    m_gateway = gateway;
    m_store = store;
    m_storage = storage;
}

bool AuthViewModel::busy() const {
    return m_busy;
}

QString AuthViewModel::error() const {
    return m_error;
}

void AuthViewModel::bootstrapSession() {
    if (!m_api || !m_storage || !m_store || !m_gateway) {
        return;
    }

    const QString refreshToken = m_storage->loadRefreshToken();
    if (refreshToken.isEmpty()) {
        return;
    }

    setBusy(true);
    m_api->refresh(refreshToken, [this](bool ok, const QJsonObject &response, const QString &error) {
        setBusy(false);
        if (!ok) {
            m_storage->clear();
            m_store->clearSession();
            setError(error);
            return;
        }

        applyAuthResponse(response);
        setError({});
    });
}

void AuthViewModel::login(const QString &email, const QString &password) {
    if (!m_api || !m_storage || !m_store || !m_gateway) {
        return;
    }

    setBusy(true);
    m_api->login(email, password, [this](bool ok, const QJsonObject &response, const QString &error) {
        setBusy(false);
        if (!ok) {
            setError(error);
            return;
        }

        applyAuthResponse(response);
        setError({});
    });
}

void AuthViewModel::registerUser(const QString &email, const QString &password, const QString &displayName) {
    if (!m_api || !m_storage || !m_store || !m_gateway) {
        return;
    }

    setBusy(true);
    m_api->registerUser(email,
                        password,
                        displayName,
                        [this](bool ok, const QJsonObject &response, const QString &error) {
                            setBusy(false);
                            if (!ok) {
                                setError(error);
                                return;
                            }

                            applyAuthResponse(response);
                            setError({});
                        });
}

void AuthViewModel::logout() {
    if (!m_api || !m_storage || !m_store || !m_gateway) {
        return;
    }

    m_api->clearTokens();
    m_storage->clear();
    m_gateway->disconnectFromGateway();
    m_store->clearSession();
    setError({});
}

void AuthViewModel::setBusy(bool busy) {
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void AuthViewModel::setError(const QString &error) {
    if (m_error == error) {
        return;
    }
    m_error = error;
    emit errorChanged();
}

void AuthViewModel::applyAuthResponse(const QJsonObject &response) {
    const QString accessToken = response.value("accessToken").toString();
    const QString refreshToken = response.value("refreshToken").toString();
    const QJsonObject user = response.value("user").toObject();

    m_api->setAccessToken(accessToken);
    m_api->setRefreshToken(refreshToken);
    m_storage->saveAuthTokens(accessToken, refreshToken);

    QVariantMap currentUser = user.toVariantMap();
    currentUser.insert(QStringLiteral("presence"), QStringLiteral("ONLINE"));
    m_store->setCurrentUser(currentUser);
    m_store->setAuthenticated(true);
    m_gateway->connectToGateway(accessToken);

    // Mark current session as online right after auth bootstrap/login.
    m_api->updatePresence(QStringLiteral("ONLINE"), [this](bool ok, const QJsonObject &presenceResponse, const QString &) {
        if (!ok || !m_store) {
            return;
        }

        QVariantMap userMap = m_store->currentUser();
        userMap.insert(QStringLiteral("presence"), presenceResponse.value(QStringLiteral("presence")).toString());
        m_store->setCurrentUser(userMap);
    });

    emit sessionReady();
}
