#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

class GatewayClient;
class QAudioSource;
class QIODevice;
class QMediaDevices;

namespace livekit {
class AudioSource;
class AudioStream;
class LocalAudioTrack;
class LocalTrackPublication;
class Room;
class RoomDelegate;
class Track;
}

class NativeVoiceEngine final : public QObject {
    Q_OBJECT

public:
    explicit NativeVoiceEngine(QObject *parent = nullptr);
    ~NativeVoiceEngine() override;

    void setGateway(GatewayClient *gateway);

    QVariantList inputDevices() const;
    QVariantList outputDevices() const;
    QString selectedInputDeviceId() const;
    QString selectedOutputDeviceId() const;

    void refreshDevices(bool requestPermission = false);
    void setSelectedInputDeviceId(const QString &deviceId);
    void setSelectedOutputDeviceId(const QString &deviceId);

    void setSessionContext(const QString &guildId, const QString &channelId);
    void clearSessionContext();
    void setSfuConnection(const QString &url,
                          const QString &token,
                          const QString &roomName,
                          const QString &identity);
    void setMuted(bool muted);
    void setDeafened(bool deafened);
    void setTransmitEnabled(bool enabled);

    void start();
    void stop();
    void playRemotePcm(const QByteArray &pcm, int sampleRate, int channels, const QString &sampleFormat);

signals:
    void inputDevicesChanged();
    void outputDevicesChanged();
    void selectedInputDeviceIdChanged();
    void selectedOutputDeviceIdChanged();
    void connectedChanged(bool connected);
    void activeSpeakersChanged(const QStringList &userIds);
    void errorChanged(const QString &error);

private:
    struct RemoteTrackState {
        std::shared_ptr<livekit::Track> track;
        QString participantId;
    };

    struct RemotePlayback {
        std::atomic_bool stop{false};
        std::shared_ptr<livekit::AudioStream> stream;
        std::thread thread;
    };

    class RoomDelegateImpl;

    void refreshDeviceLists();
    void restartInputCapture();
    void startInputCapture();
    void stopInputCapture();
    void onInputReadyRead();

    void connectRoomAsync();
    void stopRoom();
    void updateTrackMuteState();

    void handleConnected();
    void handleDisconnected(const QString &error = {});
    void handleTrackSubscribed(const std::string &trackSid,
                               const std::shared_ptr<livekit::Track> &track,
                               const QString &participantId);
    void handleTrackUnsubscribed(const std::string &trackSid);
    void handleActiveSpeakers(const QStringList &participantIds);

    void rebuildRemotePlaybacks();
    void startRemotePlayback(const std::string &trackSid,
                             const RemoteTrackState &state);
    void stopRemotePlayback(const std::string &trackSid);
    void stopAllRemotePlaybacks();

    void setConnected(bool connected);
    void setError(const QString &error);

    GatewayClient *m_gateway{nullptr};
    QMediaDevices *m_mediaDevices{nullptr};

    std::shared_ptr<livekit::Room> m_room;
    std::shared_ptr<RoomDelegateImpl> m_roomDelegate;
    std::shared_ptr<livekit::AudioSource> m_livekitAudioSource;
    std::shared_ptr<livekit::LocalAudioTrack> m_localAudioTrack;
    std::shared_ptr<livekit::LocalTrackPublication> m_localTrackPublication;

    std::thread m_connectThread;
    std::mutex m_roomMutex;
    std::mutex m_remoteAudioMutex;

    std::unordered_map<std::string, RemoteTrackState> m_remoteTracks;
    std::unordered_map<std::string, std::shared_ptr<RemotePlayback>> m_remotePlaybacks;

    QAudioSource *m_inputSource{nullptr};
    QIODevice *m_inputDevice{nullptr};
    QByteArray m_inputBuffer;

    QVariantList m_inputDevices;
    QVariantList m_outputDevices;
    QString m_selectedInputDeviceId;
    QString m_selectedOutputDeviceId;

    QString m_guildId;
    QString m_channelId;
    QString m_sfuUrl;
    QString m_sfuToken;

    bool m_muted{false};
    bool m_deafened{false};
    bool m_transmitEnabled{false};
    bool m_started{false};
    bool m_connected{false};

    std::atomic_bool m_transmittingNow{false};
    std::atomic_bool m_deafenedNow{false};
    std::atomic_uint64_t m_generation{0};

    QString m_error;
};
