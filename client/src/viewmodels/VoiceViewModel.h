#pragma once

#include <QObject>
#include <QJsonObject>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QVariantList>

class ApiClient;
class GatewayClient;
class AppStore;
class NativeVoiceEngine;

class VoiceViewModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)
    Q_PROPERTY(bool deafened READ deafened NOTIFY deafenedChanged)
    Q_PROPERTY(QString activeChannelId READ activeChannelId NOTIFY activeChannelIdChanged)
    Q_PROPERTY(QVariantList inputDevices READ inputDevices NOTIFY inputDevicesChanged)
    Q_PROPERTY(QVariantList outputDevices READ outputDevices NOTIFY outputDevicesChanged)
    Q_PROPERTY(QString selectedInputDeviceId READ selectedInputDeviceId NOTIFY selectedInputDeviceIdChanged)
    Q_PROPERTY(QString selectedOutputDeviceId READ selectedOutputDeviceId NOTIFY selectedOutputDeviceIdChanged)
    Q_PROPERTY(QString activationMode READ activationMode NOTIFY activationModeChanged)
    Q_PROPERTY(QString pttHotkey READ pttHotkey NOTIFY pttHotkeyChanged)
    Q_PROPERTY(bool pttHotkeyEnabled READ pttHotkeyEnabled NOTIFY pttHotkeyEnabledChanged)
    Q_PROPERTY(bool pttPressed READ pttPressed NOTIFY pttPressedChanged)
    Q_PROPERTY(bool audioPermissionProbeActive READ audioPermissionProbeActive NOTIFY audioPermissionProbeActiveChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit VoiceViewModel(QObject *parent = nullptr);

    void setup(ApiClient *api,
               GatewayClient *gateway,
               AppStore *store);

    bool connected() const;
    bool muted() const;
    bool deafened() const;
    QString activeChannelId() const;
    QVariantList inputDevices() const;
    QVariantList outputDevices() const;
    QString selectedInputDeviceId() const;
    QString selectedOutputDeviceId() const;
    QString activationMode() const;
    QString pttHotkey() const;
    bool pttHotkeyEnabled() const;
    bool pttPressed() const;
    bool audioPermissionProbeActive() const;
    QString error() const;

    Q_INVOKABLE void joinVoice(const QString &channelId);
    Q_INVOKABLE void leaveVoice();
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void toggleDeafen();
    Q_INVOKABLE void refreshChannelParticipants(const QString &channelId, bool force = false);
    Q_INVOKABLE void requestAudioDevices(bool requestPermission = false);
    Q_INVOKABLE void setSelectedInputDeviceId(const QString &deviceId);
    Q_INVOKABLE void setSelectedOutputDeviceId(const QString &deviceId);
    Q_INVOKABLE void setActivationMode(const QString &mode);
    Q_INVOKABLE void setPttHotkey(const QString &hotkey);
    Q_INVOKABLE void setPttHotkeyEnabled(bool enabled);
    Q_INVOKABLE void setPttPressed(bool pressed);
    Q_INVOKABLE void notifyUserGesture();

    void handleGatewayEvent(const QString &eventName, const QJsonObject &payload);

signals:
    void connectedChanged();
    void mutedChanged();
    void deafenedChanged();
    void activeChannelIdChanged();
    void inputDevicesChanged();
    void outputDevicesChanged();
    void selectedInputDeviceIdChanged();
    void selectedOutputDeviceIdChanged();
    void activationModeChanged();
    void pttHotkeyChanged();
    void pttHotkeyEnabledChanged();
    void pttPressedChanged();
    void audioPermissionProbeActiveChanged();
    void errorChanged();

private:
    void setConnected(bool connected);
    void setMuted(bool muted);
    void setDeafened(bool deafened);
    void setActiveChannelId(const QString &channelId);
    void setError(const QString &error);
    void loadParticipants();
    void fetchChannelParticipants(const QString &channelId, bool syncActiveList);
    void pruneCurrentUserFromChannel(const QString &channelId);
    void dispatchAudioSettingsToBridge();
    void restoreAudioSettings();
    void persistAudioSettings() const;
    void applyAudioDevicesFromBridge(const QVariantMap &payload);
    void setAudioPermissionProbeActive(bool active);
    void refreshAudioDevicesFromBridge(bool requestPermission = false);
    void recomputeAudioDeviceLists();

    ApiClient *m_api{nullptr};
    GatewayClient *m_gateway{nullptr};
    AppStore *m_store{nullptr};
    NativeVoiceEngine *m_nativeEngine{nullptr};
    bool m_connected{false};
    bool m_muted{false};
    bool m_deafened{false};
    bool m_leaveInProgress{false};
    QString m_joiningChannelId;
    QSet<QString> m_participantFetchInFlight;
    QHash<QString, qint64> m_lastParticipantFetchAtMs;
    QHash<QString, bool> m_pendingParticipantFetchForce;
    QString m_activeChannelId;
    QString m_error;
    QVariantList m_inputDevices;
    QVariantList m_outputDevices;
    QString m_selectedInputDeviceId;
    QString m_selectedOutputDeviceId;
    QString m_activationMode{"VOICE_ACTIVITY"};
    QString m_pttHotkey{"V"};
    bool m_pttHotkeyEnabled{true};
    bool m_pttPressed{false};
    bool m_audioPermissionProbeActive{false};
};
