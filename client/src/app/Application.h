#pragma once

#include <QObject>
#include "services/GatewayClient.h"
#include "store/AppStore.h"
#include "viewmodels/AuthViewModel.h"
#include "viewmodels/ChatViewModel.h"
#include "viewmodels/GuildViewModel.h"
#include "viewmodels/ProfileViewModel.h"
#include "viewmodels/VoiceViewModel.h"

class ApiClient;
class SecureStorage;

class Application final : public QObject {
    Q_OBJECT
    Q_PROPERTY(AppStore *store READ store CONSTANT)
    Q_PROPERTY(GatewayClient *gateway READ gateway CONSTANT)
    Q_PROPERTY(AuthViewModel *auth READ auth CONSTANT)
    Q_PROPERTY(GuildViewModel *guild READ guild CONSTANT)
    Q_PROPERTY(ChatViewModel *chat READ chat CONSTANT)
    Q_PROPERTY(VoiceViewModel *voice READ voice CONSTANT)
    Q_PROPERTY(ProfileViewModel *profile READ profile CONSTANT)

public:
    explicit Application(QObject *parent = nullptr);

    AppStore *store() const;
    GatewayClient *gateway() const;
    AuthViewModel *auth() const;
    GuildViewModel *guild() const;
    ChatViewModel *chat() const;
    VoiceViewModel *voice() const;
    ProfileViewModel *profile() const;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void shutdown();

private:
    bool m_shuttingDown{false};
    ApiClient *m_api;
    GatewayClient *m_gateway;
    SecureStorage *m_storage;
    AppStore *m_store;
    AuthViewModel *m_auth;
    GuildViewModel *m_guild;
    ChatViewModel *m_chat;
    VoiceViewModel *m_voice;
    ProfileViewModel *m_profile;
};
