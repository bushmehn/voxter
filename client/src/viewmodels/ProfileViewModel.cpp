#include "ProfileViewModel.h"

#include "services/ApiClient.h"
#include "store/AppStore.h"

ProfileViewModel::ProfileViewModel(QObject *parent)
    : QObject(parent) {}

void ProfileViewModel::setup(ApiClient *api, AppStore *store) {
    m_api = api;
    m_store = store;
}

bool ProfileViewModel::busy() const {
    return m_busy;
}

QString ProfileViewModel::error() const {
    return m_error;
}

void ProfileViewModel::saveProfile(const QString &displayName) {
    if (!m_api || !m_store) {
        return;
    }

    setError({});

    setBusy(true);
    m_api->updateProfile(displayName.trimmed(), [this](bool ok, const QJsonObject &response, const QString &error) {
        setBusy(false);
        if (!ok) {
            setError(error);
            return;
        }

        m_store->setCurrentUser(response.toVariantMap());
        setError({});
        emit saved();
    });
}

void ProfileViewModel::setBusy(bool busy) {
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void ProfileViewModel::setError(const QString &error) {
    if (m_error == error) {
        return;
    }

    m_error = error;
    emit errorChanged();
}
