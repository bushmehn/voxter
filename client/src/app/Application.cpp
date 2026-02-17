#include "Application.h"

#include <QCoreApplication>

#include "services/ApiClient.h"
#include "services/GatewayClient.h"
#include "services/SecureStorage.h"
#include "store/AppStore.h"
#include "viewmodels/AuthViewModel.h"
#include "viewmodels/ChatViewModel.h"
#include "viewmodels/GuildViewModel.h"
#include "viewmodels/ProfileViewModel.h"
#include "viewmodels/VoiceViewModel.h"

Application::Application(QObject *parent)
    : QObject(parent)
    , m_api(new ApiClient(this))
    , m_gateway(new GatewayClient(this))
    , m_storage(new SecureStorage(this))
    , m_store(new AppStore(this))
    , m_auth(new AuthViewModel(this))
    , m_guild(new GuildViewModel(this))
    , m_chat(new ChatViewModel(this))
    , m_voice(new VoiceViewModel(this))
    , m_profile(new ProfileViewModel(this)) {

    const QString apiUrl = qEnvironmentVariable("VOXTER_API_URL", QStringLiteral("http://localhost:4000"));
    m_api->setBaseUrl(apiUrl);
    m_gateway->setApiBaseUrl(apiUrl);

    m_auth->setup(m_api, m_gateway, m_store, m_storage);
    m_guild->setup(m_api, m_gateway, m_store);
    m_chat->setup(m_api, m_store);
    m_voice->setup(m_api, m_gateway, m_store);
    m_profile->setup(m_api, m_store);

    QObject::connect(m_auth, &AuthViewModel::sessionReady, this, [this]() {
        m_guild->loadGuilds();
        m_guild->loadDmThreads();
    });

    QObject::connect(m_store, &AppStore::currentChannelIdChanged, this, [this]() {
        if (m_store->currentChannelId().isEmpty()) {
            return;
        }
        m_store->setCurrentDmThreadId({});
        m_chat->loadChannelMessages(true);
    });

    QObject::connect(m_gateway,
                     &GatewayClient::eventReceived,
                     this,
                     [this](const QString &eventName, const QJsonObject &payload) {
                         if (eventName == "presence_update") {
                             const QString currentUserId = m_store->currentUser().value("id").toString();
                             if (!currentUserId.isEmpty() &&
                                 payload.value("userId").toString() == currentUserId) {
                                 QVariantMap user = m_store->currentUser();
                                 user.insert("presence", payload.value("status").toString());
                                 m_store->setCurrentUser(user);
                             }
                         }

                         m_guild->handleGatewayEvent(eventName, payload);
                         m_chat->handleGatewayEvent(eventName, payload);
                         m_voice->handleGatewayEvent(eventName, payload);
                     });
}

AppStore *Application::store() const {
    return m_store;
}

GatewayClient *Application::gateway() const {
    return m_gateway;
}

AuthViewModel *Application::auth() const {
    return m_auth;
}

GuildViewModel *Application::guild() const {
    return m_guild;
}

ChatViewModel *Application::chat() const {
    return m_chat;
}

VoiceViewModel *Application::voice() const {
    return m_voice;
}

ProfileViewModel *Application::profile() const {
    return m_profile;
}

void Application::initialize() {
    m_auth->bootstrapSession();
}

void Application::shutdown() {
    if (m_shuttingDown) {
        return;
    }
    m_shuttingDown = true;

    if (m_gateway) {
        m_gateway->disconnectFromGateway();
    }
}
