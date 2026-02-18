#include "VoiceViewModel.h"

#include "services/ApiClient.h"
#include "services/GatewayClient.h"
#include "services/NativeVoiceEngine.h"
#include "store/AppStore.h"

#include <QByteArray>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QTimer>
#include <QVariantMap>

namespace {
constexpr const char *kVoiceSettingsGroup = "voice";
constexpr const char *kInputDeviceIdKey = "inputDeviceId";
constexpr const char *kOutputDeviceIdKey = "outputDeviceId";
constexpr const char *kMicrophoneVolumeKey = "microphoneVolume";
constexpr const char *kActivationModeKey = "activationMode";
constexpr const char *kPttHotkeyKey = "pttHotkey";
constexpr const char *kPttHotkeyEnabledKey = "pttHotkeyEnabled";

QString normalizeActivationMode(const QString &mode) {
    return mode == QStringLiteral("PUSH_TO_TALK")
               ? QStringLiteral("PUSH_TO_TALK")
               : QStringLiteral("VOICE_ACTIVITY");
}

QString normalizePttHotkey(const QString &hotkey) {
    const QString trimmed = hotkey.trimmed().toUpper();
    if (trimmed.isEmpty()) {
        return QStringLiteral("V");
    }
    if (trimmed == QStringLiteral("SPACE")) {
        return QStringLiteral("SPACE");
    }
    if (trimmed.size() == 1) {
        return trimmed;
    }
    return QStringLiteral("V");
}

bool listContainsDeviceId(const QVariantList &devices, const QString &id) {
    if (id.isEmpty()) {
        return true;
    }

    for (const QVariant &entry : devices) {
        if (entry.toMap().value(QStringLiteral("id")).toString() == id) {
            return true;
        }
    }
    return false;
}

bool isTooManyRequestsError(const QString &error) {
    const QString lowered = error.toLower();
    return lowered.contains(QStringLiteral("too many requests")) ||
           lowered.contains(QStringLiteral("throttlerexception")) ||
           lowered.contains(QStringLiteral("status code 429")) ||
           lowered == QStringLiteral("429");
}

int normalizeMicrophoneVolume(int volume) {
    if (volume < 0) {
        return 0;
    }
    if (volume > 200) {
        return 200;
    }
    return volume;
}
}

VoiceViewModel::VoiceViewModel(QObject *parent)
    : QObject(parent) {
    m_nativeEngine = new NativeVoiceEngine(this);
    QObject::connect(m_nativeEngine, &NativeVoiceEngine::inputDevicesChanged, this, [this]() {
        recomputeAudioDeviceLists();
    });
    QObject::connect(m_nativeEngine, &NativeVoiceEngine::outputDevicesChanged, this, [this]() {
        recomputeAudioDeviceLists();
    });
    QObject::connect(m_nativeEngine, &NativeVoiceEngine::errorChanged, this, [this](const QString &error) {
        setError(error);
    });
    QObject::connect(m_nativeEngine, &NativeVoiceEngine::connectedChanged, this, [this](bool connected) {
        setConnected(connected);
        if (!connected) {
            setPttPressed(false);
        }
    });
    QObject::connect(m_nativeEngine, &NativeVoiceEngine::activeSpeakersChanged, this, [this](const QStringList &userIds) {
        if (!m_store || m_activeChannelId.isEmpty()) {
            return;
        }

        m_store->setVoiceSpeakingUsers(m_activeChannelId, userIds);
        m_store->setVoiceParticipants(m_store->voiceParticipantsForChannel(m_activeChannelId));
    });
    recomputeAudioDeviceLists();
}

void VoiceViewModel::setup(ApiClient *api,
                           GatewayClient *gateway,
                           AppStore *store) {
    m_api = api;
    m_gateway = gateway;
    m_store = store;
    m_nativeEngine->setGateway(gateway);
    restoreAudioSettings();
    requestAudioDevices(false);
    recomputeAudioDeviceLists();
    dispatchAudioSettingsToBridge();
}

bool VoiceViewModel::connected() const {
    return m_connected;
}

bool VoiceViewModel::muted() const {
    return m_muted;
}

bool VoiceViewModel::deafened() const {
    return m_deafened;
}

QString VoiceViewModel::activeChannelId() const {
    return m_activeChannelId;
}

QVariantList VoiceViewModel::inputDevices() const {
    return m_inputDevices;
}

QVariantList VoiceViewModel::outputDevices() const {
    return m_outputDevices;
}

QString VoiceViewModel::selectedInputDeviceId() const {
    return m_selectedInputDeviceId;
}

QString VoiceViewModel::selectedOutputDeviceId() const {
    return m_selectedOutputDeviceId;
}

int VoiceViewModel::microphoneVolume() const {
    return m_microphoneVolume;
}

QString VoiceViewModel::activationMode() const {
    return m_activationMode;
}

QString VoiceViewModel::pttHotkey() const {
    return m_pttHotkey;
}

bool VoiceViewModel::pttHotkeyEnabled() const {
    return m_pttHotkeyEnabled;
}

bool VoiceViewModel::pttPressed() const {
    return m_pttPressed;
}

bool VoiceViewModel::audioPermissionProbeActive() const {
    return m_audioPermissionProbeActive;
}

QString VoiceViewModel::error() const {
    return m_error;
}

void VoiceViewModel::joinVoice(const QString &channelId) {
    if (!m_api || !m_store) {
        setError(QStringLiteral("Voice services are not initialized"));
        return;
    }

    const QString guildId = m_store->currentGuildId();
    if (guildId.isEmpty()) {
        setError(QStringLiteral("No guild selected"));
        return;
    }

    if (channelId.isEmpty()) {
        setError(QStringLiteral("No voice channel selected"));
        return;
    }

    if (!m_joiningChannelId.isEmpty() || m_leaveInProgress) {
        return;
    }

    if (m_connected && channelId == m_activeChannelId) {
        setError({});
        return;
    }

    if (m_connected && !m_activeChannelId.isEmpty() && channelId != m_activeChannelId) {
        const QString previousChannelId = m_activeChannelId;
        m_leaveInProgress = true;

        m_api->leaveVoice(guildId, [this, channelId, previousChannelId](bool ok, const QJsonObject &, const QString &error) {
            m_leaveInProgress = false;
            if (!ok) {
                setError(error);
                return;
            }

            setConnected(false);
            setPttPressed(false);
            setActiveChannelId({});
            m_nativeEngine->stop();
            m_nativeEngine->clearSessionContext();
            m_store->setVoiceParticipants({});
            pruneCurrentUserFromChannel(previousChannelId);
            refreshChannelParticipants(previousChannelId, true);

            joinVoice(channelId);
        });
        return;
    }

    m_joiningChannelId = channelId;

    m_api->joinVoice(guildId,
                     channelId,
                     m_muted,
                     m_deafened,
                     [this, guildId, channelId](bool ok, const QJsonObject &, const QString &error) {
                         if (!ok) {
                             m_joiningChannelId.clear();
                             setError(error);
                             return;
                         }

                         m_api->issueVoiceSfuToken(guildId, channelId, [this, guildId, channelId](bool tokenOk, const QJsonObject &tokenPayload, const QString &tokenError) {
                             if (!tokenOk) {
                                 m_joiningChannelId.clear();
                                 setError(tokenError);
                                 m_api->leaveVoice(guildId, [](bool, const QJsonObject &, const QString &) {});
                                 return;
                             }

                             const QString sfuUrl = tokenPayload.value(QStringLiteral("url")).toString();
                             const QString sfuToken = tokenPayload.value(QStringLiteral("token")).toString();
                             const QString roomName = tokenPayload.value(QStringLiteral("roomName")).toString();
                             const QString identity = tokenPayload.value(QStringLiteral("identity")).toString();

                             if (sfuUrl.isEmpty() || sfuToken.isEmpty()) {
                                 m_joiningChannelId.clear();
                                 setError(QStringLiteral("voice-error:missing sfu token"));
                                 m_api->leaveVoice(guildId, [](bool, const QJsonObject &, const QString &) {});
                                 return;
                             }

                             setActiveChannelId(channelId);
                             m_joiningChannelId.clear();
                             m_nativeEngine->setSessionContext(guildId, channelId);
                             m_nativeEngine->setSfuConnection(sfuUrl, sfuToken, roomName, identity);
                             m_nativeEngine->setMuted(m_muted);
                             m_nativeEngine->setDeafened(m_deafened);
                             dispatchAudioSettingsToBridge();
                             m_nativeEngine->start();

                             loadParticipants();
                             setError({});
                         });
                     });
}

void VoiceViewModel::leaveVoice() {
    if (!m_api || !m_store || m_activeChannelId.isEmpty() || m_store->currentGuildId().isEmpty() || m_leaveInProgress ||
        !m_joiningChannelId.isEmpty()) {
        return;
    }

    const QString leftChannel = m_activeChannelId;
    m_leaveInProgress = true;
    m_api->leaveVoice(m_store->currentGuildId(), [this, leftChannel](bool ok, const QJsonObject &, const QString &error) {
        m_leaveInProgress = false;
        if (!ok) {
            setError(error);
            return;
        }

        setConnected(false);
        setPttPressed(false);
        setActiveChannelId({});
        m_nativeEngine->stop();
        m_nativeEngine->clearSessionContext();
        m_joiningChannelId.clear();
        m_store->setVoiceParticipants({});
        pruneCurrentUserFromChannel(leftChannel);
        refreshChannelParticipants(leftChannel, true);

        setError({});
    });
}

void VoiceViewModel::toggleMute() {
    if (!m_api || !m_store || !m_connected || m_store->currentGuildId().isEmpty()) {
        return;
    }

    const bool nextMuted = !m_muted;
    m_api->updateVoiceState(
        m_store->currentGuildId(),
        nextMuted,
        m_deafened,
        [this, nextMuted](bool ok, const QJsonObject &, const QString &error) {
            if (!ok) {
                setError(error);
                return;
            }
            setMuted(nextMuted);
            setError({});
        });
}

void VoiceViewModel::toggleDeafen() {
    if (!m_api || !m_store || !m_connected || m_store->currentGuildId().isEmpty()) {
        return;
    }

    const bool nextDeafened = !m_deafened;
    m_api->updateVoiceState(
        m_store->currentGuildId(),
        m_muted,
        nextDeafened,
        [this, nextDeafened](bool ok, const QJsonObject &, const QString &error) {
            if (!ok) {
                setError(error);
                return;
            }
            setDeafened(nextDeafened);
            setError({});
        });
}

void VoiceViewModel::refreshChannelParticipants(const QString &channelId, bool force) {
    if (channelId.isEmpty()) {
        return;
    }
    if (m_participantFetchInFlight.contains(channelId)) {
        const bool pendingForce = m_pendingParticipantFetchForce.value(channelId, false);
        m_pendingParticipantFetchForce.insert(channelId, pendingForce || force);
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 lastFetchMs = m_lastParticipantFetchAtMs.value(channelId, 0);
    if (!force && nowMs - lastFetchMs < 1200) {
        return;
    }

    m_lastParticipantFetchAtMs.insert(channelId, nowMs);
    fetchChannelParticipants(channelId, channelId == m_activeChannelId);
}

void VoiceViewModel::requestAudioDevices(bool requestPermission) {
    setAudioPermissionProbeActive(false);
    if (m_nativeEngine) {
        m_nativeEngine->refreshDevices(requestPermission);
    }
    recomputeAudioDeviceLists();
}

void VoiceViewModel::setSelectedInputDeviceId(const QString &deviceId) {
    if (m_selectedInputDeviceId == deviceId) {
        return;
    }

    m_selectedInputDeviceId = deviceId;
    emit selectedInputDeviceIdChanged();
    persistAudioSettings();
    dispatchAudioSettingsToBridge();
}

void VoiceViewModel::setSelectedOutputDeviceId(const QString &deviceId) {
    if (m_selectedOutputDeviceId == deviceId) {
        return;
    }

    m_selectedOutputDeviceId = deviceId;
    emit selectedOutputDeviceIdChanged();
    persistAudioSettings();
    dispatchAudioSettingsToBridge();
}

void VoiceViewModel::setMicrophoneVolume(int volume) {
    const int normalizedVolume = normalizeMicrophoneVolume(volume);
    if (m_microphoneVolume == normalizedVolume) {
        return;
    }

    m_microphoneVolume = normalizedVolume;
    emit microphoneVolumeChanged();
    persistAudioSettings();
    dispatchAudioSettingsToBridge();
}

void VoiceViewModel::setActivationMode(const QString &mode) {
    const QString normalizedMode = normalizeActivationMode(mode);
    if (m_activationMode == normalizedMode) {
        return;
    }

    m_activationMode = normalizedMode;
    emit activationModeChanged();

    if (m_activationMode != QStringLiteral("PUSH_TO_TALK") && m_pttPressed) {
        m_pttPressed = false;
        emit pttPressedChanged();
    }

    persistAudioSettings();
    dispatchAudioSettingsToBridge();
}

void VoiceViewModel::setPttHotkey(const QString &hotkey) {
    const QString normalizedHotkey = normalizePttHotkey(hotkey);
    if (m_pttHotkey == normalizedHotkey) {
        return;
    }

    m_pttHotkey = normalizedHotkey;
    emit pttHotkeyChanged();
    persistAudioSettings();
}

void VoiceViewModel::setPttHotkeyEnabled(bool enabled) {
    if (m_pttHotkeyEnabled == enabled) {
        return;
    }

    m_pttHotkeyEnabled = enabled;
    emit pttHotkeyEnabledChanged();
    persistAudioSettings();
}

void VoiceViewModel::setPttPressed(bool pressed) {
    const bool normalized = m_activationMode == QStringLiteral("PUSH_TO_TALK") && pressed;
    if (m_pttPressed == normalized) {
        return;
    }

    m_pttPressed = normalized;
    emit pttPressedChanged();
    dispatchAudioSettingsToBridge();
}

void VoiceViewModel::notifyUserGesture() {
    // No-op for native audio engine.
}

void VoiceViewModel::handleGatewayEvent(const QString &eventName, const QJsonObject &payload) {
    if (eventName == QStringLiteral("voice_audio")) {
        if (!m_store || !m_nativeEngine) {
            return;
        }

        const QString channelId = payload.value(QStringLiteral("channelId")).toString();
        if (channelId.isEmpty() || channelId != m_activeChannelId) {
            return;
        }

        const QString selfUserId = m_store->currentUser().value(QStringLiteral("id")).toString();
        const QString sourceUserId = payload.value(QStringLiteral("userId")).toString();
        if (!selfUserId.isEmpty() && sourceUserId == selfUserId) {
            return;
        }

        const QByteArray pcm = QByteArray::fromBase64(payload.value(QStringLiteral("pcm")).toString().toLatin1());
        if (pcm.isEmpty()) {
            return;
        }

        const int sampleRate = payload.value(QStringLiteral("sampleRate")).toInt(48000);
        const int channels = payload.value(QStringLiteral("channels")).toInt(1);
        const QString sampleFormat = payload.value(QStringLiteral("sampleFormat")).toString();
        m_nativeEngine->playRemotePcm(pcm, sampleRate, channels, sampleFormat);
        return;
    }

    if (eventName == QStringLiteral("voice_speaking_update")) {
        if (!m_store) {
            return;
        }

        const QString channelId = payload.value(QStringLiteral("channelId")).toString();
        const QString userId = payload.value(QStringLiteral("userId")).toString();
        if (channelId.isEmpty() || userId.isEmpty()) {
            return;
        }

        const bool speaking = payload.value(QStringLiteral("speaking")).toBool();
        m_store->updateVoiceParticipantState(channelId, userId, {{QStringLiteral("speaking"), speaking}});

        if (channelId == m_activeChannelId) {
            m_store->setVoiceParticipants(m_store->voiceParticipantsForChannel(channelId));
        }
        return;
    }

    if (eventName != QStringLiteral("voice_state_update")) {
        return;
    }

    if (!m_store) {
        return;
    }

    const QString channelId = payload.value(QStringLiteral("channelId")).toString();
    if (channelId.isEmpty()) {
        return;
    }

    const QString selfUserId = m_store->currentUser().value(QStringLiteral("id")).toString();
    const QString userId = payload.value(QStringLiteral("userId")).toString();
    const QString action = payload.value(QStringLiteral("action")).toString();

    if (!userId.isEmpty()) {
        if (action == QStringLiteral("leave")) {
            m_store->updateVoiceParticipantState(channelId, userId, {}, true);
        } else {
            QVariantMap patch;
            if (payload.contains(QStringLiteral("muted"))) {
                patch.insert(QStringLiteral("muted"), payload.value(QStringLiteral("muted")).toBool());
            }
            if (payload.contains(QStringLiteral("deafened"))) {
                patch.insert(QStringLiteral("deafened"), payload.value(QStringLiteral("deafened")).toBool());
            }
            if (payload.contains(QStringLiteral("user")) && payload.value(QStringLiteral("user")).isObject()) {
                patch.insert(QStringLiteral("user"), payload.value(QStringLiteral("user")).toObject().toVariantMap());
            }
            if (!patch.isEmpty()) {
                patch.insert(QStringLiteral("speaking"), false);
                if (action == QStringLiteral("join")) {
                    m_store->removeVoiceParticipantFromAllChannels(userId, channelId);
                }
                m_store->updateVoiceParticipantState(channelId, userId, patch);
            }
        }
    }

    const bool isSelfEvent = !selfUserId.isEmpty() && userId == selfUserId;
    if (isSelfEvent && action == QStringLiteral("leave")) {
        setConnected(false);
        setPttPressed(false);
        m_nativeEngine->stop();
        m_nativeEngine->clearSessionContext();
        setActiveChannelId({});
    }
    const bool forceRefresh = isSelfEvent;
    refreshChannelParticipants(channelId, forceRefresh);

    if (channelId == m_activeChannelId) {
        m_store->setVoiceParticipants(m_store->voiceParticipantsForChannel(channelId));
    }
}

void VoiceViewModel::setConnected(bool connected) {
    if (m_connected == connected) {
        return;
    }

    m_connected = connected;
    if (!m_connected && m_pttPressed) {
        m_pttPressed = false;
        emit pttPressedChanged();
    }
    dispatchAudioSettingsToBridge();
    emit connectedChanged();
}

void VoiceViewModel::setMuted(bool muted) {
    if (m_muted == muted) {
        return;
    }

    m_muted = muted;
    dispatchAudioSettingsToBridge();
    emit mutedChanged();
}

void VoiceViewModel::setDeafened(bool deafened) {
    if (m_deafened == deafened) {
        return;
    }

    m_deafened = deafened;
    dispatchAudioSettingsToBridge();
    emit deafenedChanged();
}

void VoiceViewModel::setActiveChannelId(const QString &channelId) {
    if (m_activeChannelId == channelId) {
        return;
    }

    m_activeChannelId = channelId;
    emit activeChannelIdChanged();
}

void VoiceViewModel::setError(const QString &error) {
    if (m_error == error) {
        return;
    }

    m_error = error;
    emit errorChanged();
}

void VoiceViewModel::loadParticipants() {
    fetchChannelParticipants(m_activeChannelId, true);
}

void VoiceViewModel::fetchChannelParticipants(const QString &channelId, bool syncActiveList) {
    if (!m_api || !m_store || channelId.isEmpty()) {
        return;
    }
    if (m_participantFetchInFlight.contains(channelId)) {
        return;
    }

    m_participantFetchInFlight.insert(channelId);

    m_api->listVoiceParticipants(channelId, [this, channelId, syncActiveList](bool ok, const QJsonArray &participants, const QString &error) {
        m_participantFetchInFlight.remove(channelId);
        const bool pendingForce = m_pendingParticipantFetchForce.take(channelId);

        if (!ok) {
            if (isTooManyRequestsError(error)) {
                QTimer::singleShot(900, this, [this, channelId]() {
                    refreshChannelParticipants(channelId, true);
                });
                return;
            }
            setError(error);
            if (pendingForce) {
                refreshChannelParticipants(channelId, true);
            }
            return;
        }

        const QVariantList participantList = participants.toVariantList();
        m_store->setVoiceParticipantsForChannel(channelId, participantList);
        if (syncActiveList && channelId == m_activeChannelId) {
            m_store->setVoiceParticipants(participantList);
        }

        setError({});

        if (pendingForce) {
            refreshChannelParticipants(channelId, true);
        }
    });
}

void VoiceViewModel::pruneCurrentUserFromChannel(const QString &channelId) {
    if (!m_store || channelId.isEmpty()) {
        return;
    }

    const QString selfUserId = m_store->currentUser().value(QStringLiteral("id")).toString();
    if (selfUserId.isEmpty()) {
        return;
    }

    const QVariantList participants = m_store->voiceParticipantsForChannel(channelId);
    if (participants.isEmpty()) {
        return;
    }

    QVariantList filtered;
    filtered.reserve(participants.size());
    bool changed = false;

    for (const QVariant &entry : participants) {
        const QVariantMap participant = entry.toMap();
        const QVariantMap participantUser = participant.value(QStringLiteral("user")).toMap();
        const QString userId = participantUser.value(QStringLiteral("id")).toString().isEmpty()
                                   ? participant.value(QStringLiteral("userId")).toString()
                                   : participantUser.value(QStringLiteral("id")).toString();

        if (userId == selfUserId) {
            changed = true;
            continue;
        }

        filtered.append(participant);
    }

    if (changed) {
        m_store->setVoiceParticipantsForChannel(channelId, filtered);
    }
}

void VoiceViewModel::dispatchAudioSettingsToBridge() {
    if (!m_nativeEngine) {
        return;
    }

    m_nativeEngine->setSelectedInputDeviceId(m_selectedInputDeviceId);
    m_nativeEngine->setSelectedOutputDeviceId(m_selectedOutputDeviceId);
    m_nativeEngine->setMicrophoneGain(static_cast<qreal>(m_microphoneVolume) / 100.0);
    m_nativeEngine->setMuted(m_muted);
    m_nativeEngine->setDeafened(m_deafened);

    bool transmitEnabled = m_connected && !m_activeChannelId.isEmpty() && !m_muted && !m_deafened;
    if (m_activationMode == QStringLiteral("PUSH_TO_TALK")) {
        transmitEnabled = transmitEnabled && m_pttPressed;
    }
    m_nativeEngine->setTransmitEnabled(transmitEnabled);
}

void VoiceViewModel::restoreAudioSettings() {
    QSettings settings(QStringLiteral("Voxter"), QStringLiteral("VoxterDesktop"));
    settings.beginGroup(QString::fromLatin1(kVoiceSettingsGroup));
    m_selectedInputDeviceId = settings.value(QString::fromLatin1(kInputDeviceIdKey)).toString();
    m_selectedOutputDeviceId = settings.value(QString::fromLatin1(kOutputDeviceIdKey)).toString();
    m_microphoneVolume = normalizeMicrophoneVolume(
        settings.value(QString::fromLatin1(kMicrophoneVolumeKey), 100).toInt());
    m_activationMode = normalizeActivationMode(
        settings.value(QString::fromLatin1(kActivationModeKey), QStringLiteral("VOICE_ACTIVITY")).toString());
    m_pttHotkey = normalizePttHotkey(
        settings.value(QString::fromLatin1(kPttHotkeyKey), QStringLiteral("V")).toString());
    m_pttHotkeyEnabled = settings.value(QString::fromLatin1(kPttHotkeyEnabledKey), true).toBool();
    settings.endGroup();
}

void VoiceViewModel::persistAudioSettings() const {
    QSettings settings(QStringLiteral("Voxter"), QStringLiteral("VoxterDesktop"));
    settings.beginGroup(QString::fromLatin1(kVoiceSettingsGroup));
    settings.setValue(QString::fromLatin1(kInputDeviceIdKey), m_selectedInputDeviceId);
    settings.setValue(QString::fromLatin1(kOutputDeviceIdKey), m_selectedOutputDeviceId);
    settings.setValue(QString::fromLatin1(kMicrophoneVolumeKey), m_microphoneVolume);
    settings.setValue(QString::fromLatin1(kActivationModeKey), m_activationMode);
    settings.setValue(QString::fromLatin1(kPttHotkeyKey), m_pttHotkey);
    settings.setValue(QString::fromLatin1(kPttHotkeyEnabledKey), m_pttHotkeyEnabled);
    settings.endGroup();
    settings.sync();
}

void VoiceViewModel::applyAudioDevicesFromBridge(const QVariantMap &payload) {
    Q_UNUSED(payload);
    recomputeAudioDeviceLists();

    if (m_audioPermissionProbeActive) {
        setAudioPermissionProbeActive(false);
    }

    dispatchAudioSettingsToBridge();
}

void VoiceViewModel::setAudioPermissionProbeActive(bool active) {
    if (m_audioPermissionProbeActive == active) {
        return;
    }

    m_audioPermissionProbeActive = active;
    emit audioPermissionProbeActiveChanged();
}

void VoiceViewModel::refreshAudioDevicesFromBridge(bool requestPermission) {
    if (!m_nativeEngine) {
        return;
    }
    m_nativeEngine->refreshDevices(requestPermission);
    recomputeAudioDeviceLists();
    setAudioPermissionProbeActive(false);
}

void VoiceViewModel::recomputeAudioDeviceLists() {
    if (!m_nativeEngine) {
        return;
    }

    QVariantList nextInputs;
    QVariantList nextOutputs;

    nextInputs = m_nativeEngine->inputDevices();
    nextOutputs = m_nativeEngine->outputDevices();

    bool settingsChanged = false;
    if (!listContainsDeviceId(nextInputs, m_selectedInputDeviceId)) {
        m_selectedInputDeviceId.clear();
        emit selectedInputDeviceIdChanged();
        settingsChanged = true;
    }

    if (!listContainsDeviceId(nextOutputs, m_selectedOutputDeviceId)) {
        m_selectedOutputDeviceId.clear();
        emit selectedOutputDeviceIdChanged();
        settingsChanged = true;
    }

    if (m_inputDevices != nextInputs) {
        m_inputDevices = nextInputs;
        emit inputDevicesChanged();
    }

    if (m_outputDevices != nextOutputs) {
        m_outputDevices = nextOutputs;
        emit outputDevicesChanged();
    }

    if (settingsChanged) {
        persistAudioSettings();
    }
}
