#include "NativeVoiceEngine.h"

#include "GatewayClient.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>
#include <QMetaObject>
#include <QVariantMap>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <exception>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <livekit/audio_frame.h>
#include <livekit/audio_source.h>
#include <livekit/audio_stream.h>
#include <livekit/livekit.h>
#include <livekit/local_audio_track.h>
#include <livekit/local_participant.h>
#include <livekit/room.h>
#include <livekit/room_delegate.h>
#include <livekit/room_event_types.h>
#include <livekit/track.h>

namespace {
constexpr int kCaptureSampleRate = 48000;
constexpr int kCaptureChannels = 1;
constexpr int kCaptureFrameMs = 10;
constexpr int kCaptureSamplesPerChannel = (kCaptureSampleRate * kCaptureFrameMs) / 1000;
constexpr int kCaptureFrameBytes = kCaptureSamplesPerChannel * kCaptureChannels * static_cast<int>(sizeof(int16_t));

QString encodeDeviceId(const QByteArray &rawId) {
    return QString::fromLatin1(rawId.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QByteArray decodeDeviceId(const QString &encodedId) {
    if (encodedId.isEmpty()) {
        return {};
    }
    return QByteArray::fromBase64(encodedId.toLatin1(), QByteArray::Base64UrlEncoding);
}

QVariantList buildInputDeviceList() {
    QVariantList devices;
    devices.append(QVariantMap{
        {QStringLiteral("id"), QString()},
        {QStringLiteral("label"), QStringLiteral("System Default")},
    });

    const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    for (const QAudioDevice &device : inputs) {
        const QString encodedId = encodeDeviceId(device.id());
        if (encodedId.isEmpty()) {
            continue;
        }
        devices.append(QVariantMap{
            {QStringLiteral("id"), encodedId},
            {QStringLiteral("label"), device.description().isEmpty() ? QStringLiteral("Microphone") : device.description()},
        });
    }

    return devices;
}

QVariantList buildOutputDeviceList() {
    QVariantList devices;
    devices.append(QVariantMap{
        {QStringLiteral("id"), QString()},
        {QStringLiteral("label"), QStringLiteral("System Default")},
    });

    const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
    for (const QAudioDevice &device : outputs) {
        const QString encodedId = encodeDeviceId(device.id());
        if (encodedId.isEmpty()) {
            continue;
        }
        devices.append(QVariantMap{
            {QStringLiteral("id"), encodedId},
            {QStringLiteral("label"), device.description().isEmpty() ? QStringLiteral("Speaker") : device.description()},
        });
    }

    return devices;
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

QAudioDevice resolveInputDevice(const QString &selectedId) {
    if (!selectedId.isEmpty()) {
        const QByteArray rawId = decodeDeviceId(selectedId);
        const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
        for (const QAudioDevice &device : inputs) {
            if (device.id() == rawId) {
                return device;
            }
        }
    }
    return QMediaDevices::defaultAudioInput();
}

QAudioDevice resolveOutputDevice(const QString &selectedId) {
    if (!selectedId.isEmpty()) {
        const QByteArray rawId = decodeDeviceId(selectedId);
        const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
        for (const QAudioDevice &device : outputs) {
            if (device.id() == rawId) {
                return device;
            }
        }
    }
    return QMediaDevices::defaultAudioOutput();
}

qreal normalizeMicrophoneGain(qreal gain) {
    if (gain < 0.0) {
        return 0.0;
    }
    if (gain > 3.0) {
        return 3.0;
    }
    return gain;
}
}

class NativeVoiceEngine::RoomDelegateImpl final : public livekit::RoomDelegate {
public:
    RoomDelegateImpl(NativeVoiceEngine *engine, uint64_t generation)
        : m_engine(engine)
        , m_generation(generation) {}

    void onConnectionStateChanged(livekit::Room &, const livekit::ConnectionStateChangedEvent &event) override {
        if (!m_engine) {
            return;
        }

        const livekit::ConnectionState state = event.state;
        QMetaObject::invokeMethod(
            m_engine,
            [engine = m_engine, generation = m_generation, state]() {
                if (generation != engine->m_generation.load()) {
                    return;
                }

                if (state == livekit::ConnectionState::Connected) {
                    engine->handleConnected();
                } else if (state == livekit::ConnectionState::Disconnected) {
                    engine->handleDisconnected(QStringLiteral("voice-disconnected"));
                } else {
                    engine->setConnected(false);
                }
            },
            Qt::QueuedConnection);
    }

    void onDisconnected(livekit::Room &, const livekit::DisconnectedEvent &) override {
        if (!m_engine) {
            return;
        }
        QMetaObject::invokeMethod(
            m_engine,
            [engine = m_engine, generation = m_generation]() {
                if (generation != engine->m_generation.load()) {
                    return;
                }
                engine->handleDisconnected(QStringLiteral("voice-disconnected"));
            },
            Qt::QueuedConnection);
    }

    void onTrackSubscribed(livekit::Room &, const livekit::TrackSubscribedEvent &event) override {
        if (!m_engine || !event.track || !event.participant) {
            return;
        }
        if (event.track->kind() != livekit::TrackKind::KIND_AUDIO) {
            return;
        }

        const std::string trackSid = event.track->sid();
        const std::shared_ptr<livekit::Track> track = event.track;
        const QString participantId = QString::fromStdString(event.participant->identity());
        QMetaObject::invokeMethod(
            m_engine,
            [engine = m_engine, generation = m_generation, trackSid, track, participantId]() {
                if (generation != engine->m_generation.load()) {
                    return;
                }
                engine->handleTrackSubscribed(trackSid, track, participantId);
            },
            Qt::QueuedConnection);
    }

    void onTrackUnsubscribed(livekit::Room &, const livekit::TrackUnsubscribedEvent &event) override {
        if (!m_engine) {
            return;
        }

        std::string trackSid;
        if (event.track) {
            trackSid = event.track->sid();
        } else if (event.publication) {
            trackSid = event.publication->sid();
        }
        if (trackSid.empty()) {
            return;
        }

        QMetaObject::invokeMethod(
            m_engine,
            [engine = m_engine, generation = m_generation, trackSid]() {
                if (generation != engine->m_generation.load()) {
                    return;
                }
                engine->handleTrackUnsubscribed(trackSid);
            },
            Qt::QueuedConnection);
    }

    void onActiveSpeakersChanged(livekit::Room &, const livekit::ActiveSpeakersChangedEvent &event) override {
        if (!m_engine) {
            return;
        }

        QStringList speakerIds;
        speakerIds.reserve(static_cast<qsizetype>(event.speakers.size()));
        for (livekit::Participant *participant : event.speakers) {
            if (!participant) {
                continue;
            }
            const QString participantId = QString::fromStdString(participant->identity());
            if (!participantId.isEmpty()) {
                speakerIds.append(participantId);
            }
        }

        QMetaObject::invokeMethod(
            m_engine,
            [engine = m_engine, generation = m_generation, speakerIds]() {
                if (generation != engine->m_generation.load()) {
                    return;
                }
                engine->handleActiveSpeakers(speakerIds);
            },
            Qt::QueuedConnection);
    }

private:
    NativeVoiceEngine *m_engine{nullptr};
    uint64_t m_generation{0};
};

NativeVoiceEngine::NativeVoiceEngine(QObject *parent)
    : QObject(parent)
    , m_mediaDevices(new QMediaDevices(this)) {
    static std::once_flag livekitInitOnce;
    std::call_once(livekitInitOnce, []() {
        livekit::initialize(livekit::LogSink::kCallback);
    });

    QObject::connect(m_mediaDevices, &QMediaDevices::audioInputsChanged, this, [this]() {
        refreshDeviceLists();
        if (m_started) {
            restartInputCapture();
        }
    });
    QObject::connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, [this]() {
        refreshDeviceLists();
        if (m_started) {
            rebuildRemotePlaybacks();
        }
    });

    refreshDeviceLists();
}

NativeVoiceEngine::~NativeVoiceEngine() {
    stop();
}

void NativeVoiceEngine::setGateway(GatewayClient *gateway) {
    m_gateway = gateway;
}

QVariantList NativeVoiceEngine::inputDevices() const {
    return m_inputDevices;
}

QVariantList NativeVoiceEngine::outputDevices() const {
    return m_outputDevices;
}

QString NativeVoiceEngine::selectedInputDeviceId() const {
    return m_selectedInputDeviceId;
}

QString NativeVoiceEngine::selectedOutputDeviceId() const {
    return m_selectedOutputDeviceId;
}

void NativeVoiceEngine::refreshDevices(bool requestPermission) {
    Q_UNUSED(requestPermission);
    refreshDeviceLists();
}

void NativeVoiceEngine::setSelectedInputDeviceId(const QString &deviceId) {
    if (m_selectedInputDeviceId == deviceId) {
        return;
    }

    m_selectedInputDeviceId = deviceId;
    emit selectedInputDeviceIdChanged();

    if (m_started) {
        restartInputCapture();
    }
}

void NativeVoiceEngine::setSelectedOutputDeviceId(const QString &deviceId) {
    if (m_selectedOutputDeviceId == deviceId) {
        return;
    }

    m_selectedOutputDeviceId = deviceId;
    emit selectedOutputDeviceIdChanged();

    if (m_started) {
        rebuildRemotePlaybacks();
    }
}

void NativeVoiceEngine::setSessionContext(const QString &guildId, const QString &channelId) {
    m_guildId = guildId;
    m_channelId = channelId;
}

void NativeVoiceEngine::clearSessionContext() {
    m_guildId.clear();
    m_channelId.clear();
}

void NativeVoiceEngine::setSfuConnection(const QString &url,
                                         const QString &token,
                                         const QString &roomName,
                                         const QString &identity) {
    Q_UNUSED(roomName);
    Q_UNUSED(identity);
    m_sfuUrl = url;
    m_sfuToken = token;
}

void NativeVoiceEngine::setMicrophoneGain(qreal gain) {
    const qreal normalizedGain = normalizeMicrophoneGain(gain);
    if (qFuzzyCompare(m_microphoneGain, normalizedGain)) {
        return;
    }
    m_microphoneGain = normalizedGain;
}

void NativeVoiceEngine::setMuted(bool muted) {
    if (m_muted == muted) {
        return;
    }
    m_muted = muted;
    updateTrackMuteState();
}

void NativeVoiceEngine::setDeafened(bool deafened) {
    if (m_deafened == deafened) {
        return;
    }
    m_deafened = deafened;
    m_deafenedNow.store(deafened);
    updateTrackMuteState();
}

void NativeVoiceEngine::setTransmitEnabled(bool enabled) {
    if (m_transmitEnabled == enabled) {
        return;
    }
    m_transmitEnabled = enabled;
    updateTrackMuteState();
}

void NativeVoiceEngine::start() {
    if (m_started) {
        updateTrackMuteState();
        return;
    }

    if (m_sfuUrl.isEmpty() || m_sfuToken.isEmpty()) {
        setError(QStringLiteral("Missing SFU token"));
        return;
    }

    m_started = true;
    m_generation.fetch_add(1);
    setError({});
    connectRoomAsync();
}

void NativeVoiceEngine::stop() {
    m_started = false;
    m_transmittingNow.store(false);
    m_generation.fetch_add(1);

    stopRoom();
    setConnected(false);
    emit activeSpeakersChanged({});
}

void NativeVoiceEngine::playRemotePcm(const QByteArray &pcm, int sampleRate, int channels, const QString &sampleFormat) {
    Q_UNUSED(pcm);
    Q_UNUSED(sampleRate);
    Q_UNUSED(channels);
    Q_UNUSED(sampleFormat);
}

void NativeVoiceEngine::refreshDeviceLists() {
    const QVariantList nextInputs = buildInputDeviceList();
    const QVariantList nextOutputs = buildOutputDeviceList();

    bool selectedChanged = false;

    if (!listContainsDeviceId(nextInputs, m_selectedInputDeviceId)) {
        m_selectedInputDeviceId.clear();
        selectedChanged = true;
        emit selectedInputDeviceIdChanged();
    }

    if (!listContainsDeviceId(nextOutputs, m_selectedOutputDeviceId)) {
        m_selectedOutputDeviceId.clear();
        selectedChanged = true;
        emit selectedOutputDeviceIdChanged();
    }

    if (m_inputDevices != nextInputs) {
        m_inputDevices = nextInputs;
        emit inputDevicesChanged();
    }

    if (m_outputDevices != nextOutputs) {
        m_outputDevices = nextOutputs;
        emit outputDevicesChanged();
    }

    if (selectedChanged && m_started) {
        restartInputCapture();
        rebuildRemotePlaybacks();
    }
}

void NativeVoiceEngine::restartInputCapture() {
    stopInputCapture();
    startInputCapture();
}

void NativeVoiceEngine::startInputCapture() {
    if (!m_started || !m_connected) {
        return;
    }

    std::shared_ptr<livekit::AudioSource> source;
    {
        std::lock_guard<std::mutex> lock(m_roomMutex);
        source = m_livekitAudioSource;
    }
    if (!source) {
        return;
    }

    QAudioFormat format;
    format.setSampleRate(kCaptureSampleRate);
    format.setChannelCount(kCaptureChannels);
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice inputDevice = resolveInputDevice(m_selectedInputDeviceId);
    if (!inputDevice.isNull() && !inputDevice.isFormatSupported(format)) {
        setError(QStringLiteral("voice-error:microphone format is not supported"));
        return;
    }

    m_inputSource = new QAudioSource(inputDevice, format, this);
    m_inputDevice = m_inputSource->start();
    if (!m_inputDevice) {
        setError(QStringLiteral("voice-error:failed to start microphone"));
        delete m_inputSource;
        m_inputSource = nullptr;
        return;
    }

    m_inputBuffer.clear();
    QObject::connect(m_inputDevice, &QIODevice::readyRead, this, &NativeVoiceEngine::onInputReadyRead);
}

void NativeVoiceEngine::stopInputCapture() {
    if (m_inputDevice) {
        m_inputDevice->disconnect(this);
        m_inputDevice = nullptr;
    }

    if (m_inputSource) {
        m_inputSource->stop();
        delete m_inputSource;
        m_inputSource = nullptr;
    }

    m_inputBuffer.clear();
}

void NativeVoiceEngine::onInputReadyRead() {
    if (!m_inputDevice) {
        return;
    }

    const QByteArray chunk = m_inputDevice->readAll();
    if (chunk.isEmpty()) {
        return;
    }
    m_inputBuffer.append(chunk);

    std::shared_ptr<livekit::AudioSource> source;
    {
        std::lock_guard<std::mutex> lock(m_roomMutex);
        source = m_livekitAudioSource;
    }
    if (!source) {
        m_inputBuffer.clear();
        return;
    }

    const int frameCount = m_inputBuffer.size() / kCaptureFrameBytes;
    if (frameCount <= 0) {
        return;
    }

    const bool transmitNow = m_transmittingNow.load();
    const qreal gain = m_microphoneGain;

    if (!transmitNow) {
        m_inputBuffer.remove(0, frameCount * kCaptureFrameBytes);
        return;
    }

    const char *raw = m_inputBuffer.constData();
    const size_t pcmSampleCount = static_cast<size_t>(kCaptureSamplesPerChannel * kCaptureChannels);

    for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        const char *framePtr = raw + (frameIndex * kCaptureFrameBytes);
        std::vector<int16_t> pcm(pcmSampleCount);
        std::memcpy(pcm.data(), framePtr, static_cast<size_t>(kCaptureFrameBytes));

        if (!qFuzzyCompare(gain, 1.0)) {
            for (int16_t &sample : pcm) {
                const qreal scaled = static_cast<qreal>(sample) * gain;
                if (scaled > 32767.0) {
                    sample = 32767;
                } else if (scaled < -32768.0) {
                    sample = -32768;
                } else {
                    sample = static_cast<int16_t>(scaled);
                }
            }
        }

        try {
            livekit::AudioFrame frame(std::move(pcm), kCaptureSampleRate, kCaptureChannels, kCaptureSamplesPerChannel);
            // 0 means wait until the SDK consumes the frame (no artificial timeout).
            source->captureFrame(frame, 0);
        } catch (const std::exception &e) {
            const QString message = QString::fromUtf8(e.what());
            if (message.contains(QStringLiteral("timed out"), Qt::CaseInsensitive)) {
                continue;
            }
            setError(QStringLiteral("voice-error:microphone capture failed: %1").arg(message));
            return;
        } catch (...) {
            setError(QStringLiteral("voice-error:microphone capture failed"));
            return;
        }
    }

    m_inputBuffer.remove(0, frameCount * kCaptureFrameBytes);
}

void NativeVoiceEngine::connectRoomAsync() {
    if (m_connectThread.joinable()) {
        m_connectThread.join();
    }

    const uint64_t generation = m_generation.load();
    const QString url = m_sfuUrl;
    const QString token = m_sfuToken;

    m_connectThread = std::thread([this, generation, url, token]() {
        auto room = std::make_shared<livekit::Room>();
        auto delegate = std::make_shared<RoomDelegateImpl>(this, generation);
        room->setDelegate(delegate.get());

        livekit::RoomOptions options;
        options.auto_subscribe = true;

        QString error;
        bool connected = false;
        try {
            connected = room->Connect(url.toStdString(), token.toStdString(), options);
        } catch (const std::exception &e) {
            error = QStringLiteral("voice-error:%1").arg(QString::fromUtf8(e.what()));
        } catch (...) {
            error = QStringLiteral("voice-error:failed to connect to LiveKit");
        }

        if (!connected && error.isEmpty()) {
            error = QStringLiteral("voice-error:failed to connect to LiveKit");
        }

        if (!error.isEmpty()) {
            QMetaObject::invokeMethod(
                this,
                [this, generation, error]() {
                    if (generation != m_generation.load()) {
                        return;
                    }
                    handleDisconnected(error);
                },
                Qt::QueuedConnection);
            return;
        }

        std::shared_ptr<livekit::AudioSource> audioSource;
        std::shared_ptr<livekit::LocalAudioTrack> localTrack;
        std::shared_ptr<livekit::LocalTrackPublication> localPublication;
        try {
            audioSource = std::make_shared<livekit::AudioSource>(kCaptureSampleRate, kCaptureChannels, 0);
            localTrack = livekit::LocalAudioTrack::createLocalAudioTrack("voxter-mic", audioSource);

            livekit::TrackPublishOptions publishOptions;
            publishOptions.source = livekit::TrackSource::SOURCE_MICROPHONE;
            localPublication = room->localParticipant()->publishTrack(localTrack, publishOptions);
        } catch (const std::exception &e) {
            error = QStringLiteral("voice-error:%1").arg(QString::fromUtf8(e.what()));
        } catch (...) {
            error = QStringLiteral("voice-error:failed to publish microphone track");
        }

        QMetaObject::invokeMethod(
            this,
            [this, generation, room, delegate, audioSource, localTrack, localPublication, error]() mutable {
                if (generation != m_generation.load()) {
                    return;
                }

                if (!error.isEmpty()) {
                    handleDisconnected(error);
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(m_roomMutex);
                    m_room = std::move(room);
                    m_roomDelegate = std::move(delegate);
                    m_livekitAudioSource = std::move(audioSource);
                    m_localAudioTrack = std::move(localTrack);
                    m_localTrackPublication = std::move(localPublication);
                }

                handleConnected();
            },
            Qt::QueuedConnection);
    });
}

void NativeVoiceEngine::stopRoom() {
    stopInputCapture();
    stopAllRemotePlaybacks();

    {
        std::lock_guard<std::mutex> audioLock(m_remoteAudioMutex);
        m_remoteTracks.clear();
    }

    {
        std::lock_guard<std::mutex> roomLock(m_roomMutex);
        m_localTrackPublication.reset();
        m_localAudioTrack.reset();
        m_livekitAudioSource.reset();
        m_roomDelegate.reset();
        m_room.reset();
    }

    if (m_connectThread.joinable()) {
        m_connectThread.join();
    }
}

void NativeVoiceEngine::updateTrackMuteState() {
    const bool shouldTransmit =
        m_started && m_connected && m_transmitEnabled && !m_muted && !m_deafened;
    m_transmittingNow.store(shouldTransmit);
    m_deafenedNow.store(m_deafened);

    std::shared_ptr<livekit::LocalAudioTrack> localTrack;
    {
        std::lock_guard<std::mutex> lock(m_roomMutex);
        localTrack = m_localAudioTrack;
    }

    if (!localTrack) {
        return;
    }

    try {
        if (shouldTransmit) {
            localTrack->unmute();
        } else {
            localTrack->mute();
        }
    } catch (const std::exception &e) {
        setError(QStringLiteral("voice-error:%1").arg(QString::fromUtf8(e.what())));
    } catch (...) {
        setError(QStringLiteral("voice-error:failed to update microphone state"));
    }
}

void NativeVoiceEngine::handleConnected() {
    if (!m_started) {
        return;
    }

    setConnected(true);
    setError({});
    startInputCapture();
    updateTrackMuteState();
}

void NativeVoiceEngine::handleDisconnected(const QString &error) {
    stopInputCapture();
    stopAllRemotePlaybacks();

    {
        std::lock_guard<std::mutex> audioLock(m_remoteAudioMutex);
        m_remoteTracks.clear();
    }

    {
        std::lock_guard<std::mutex> roomLock(m_roomMutex);
        m_localTrackPublication.reset();
        m_localAudioTrack.reset();
        m_livekitAudioSource.reset();
        m_roomDelegate.reset();
        m_room.reset();
    }

    m_started = false;
    m_transmittingNow.store(false);
    setConnected(false);
    emit activeSpeakersChanged({});
    if (!error.isEmpty()) {
        setError(error);
    }
}

void NativeVoiceEngine::handleTrackSubscribed(const std::string &trackSid,
                                              const std::shared_ptr<livekit::Track> &track,
                                              const QString &participantId) {
    if (trackSid.empty() || !track) {
        return;
    }

    RemoteTrackState state;
    state.track = track;
    state.participantId = participantId;

    {
        std::lock_guard<std::mutex> lock(m_remoteAudioMutex);
        m_remoteTracks[trackSid] = state;
    }

    startRemotePlayback(trackSid, state);
}

void NativeVoiceEngine::handleTrackUnsubscribed(const std::string &trackSid) {
    if (trackSid.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_remoteAudioMutex);
        m_remoteTracks.erase(trackSid);
    }

    stopRemotePlayback(trackSid);
}

void NativeVoiceEngine::handleActiveSpeakers(const QStringList &participantIds) {
    emit activeSpeakersChanged(participantIds);
}

void NativeVoiceEngine::rebuildRemotePlaybacks() {
    std::vector<std::pair<std::string, RemoteTrackState>> tracks;
    {
        std::lock_guard<std::mutex> lock(m_remoteAudioMutex);
        tracks.reserve(m_remoteTracks.size());
        for (const auto &[trackSid, state] : m_remoteTracks) {
            tracks.emplace_back(trackSid, state);
        }
    }

    stopAllRemotePlaybacks();
    for (const auto &[trackSid, state] : tracks) {
        startRemotePlayback(trackSid, state);
    }
}

void NativeVoiceEngine::startRemotePlayback(const std::string &trackSid,
                                            const RemoteTrackState &state) {
    stopRemotePlayback(trackSid);

    if (!state.track || state.track->kind() != livekit::TrackKind::KIND_AUDIO) {
        return;
    }

    livekit::AudioStream::Options options;
    options.capacity = 64;

    std::shared_ptr<livekit::AudioStream> stream;
    try {
        stream = livekit::AudioStream::fromTrack(state.track, options);
    } catch (const std::exception &e) {
        setError(QStringLiteral("voice-error:%1").arg(QString::fromUtf8(e.what())));
        return;
    } catch (...) {
        setError(QStringLiteral("voice-error:failed to start remote audio stream"));
        return;
    }

    if (!stream) {
        setError(QStringLiteral("voice-error:failed to start remote audio stream"));
        return;
    }

    auto playback = std::make_shared<RemotePlayback>();
    playback->stream = stream;
    const QAudioDevice outputDevice = resolveOutputDevice(m_selectedOutputDeviceId);
    playback->thread = std::thread([this, playback, outputDevice]() {
        std::unique_ptr<QAudioSink> sink;
        QIODevice *sinkDevice = nullptr;
        int activeRate = 0;
        int activeChannels = 0;

        livekit::AudioFrameEvent event;
        while (!playback->stop.load()) {
            if (!playback->stream->read(event)) {
                break;
            }
            if (playback->stop.load()) {
                break;
            }

            const livekit::AudioFrame &frame = event.frame;
            if (frame.total_samples() == 0) {
                continue;
            }
            if (m_deafenedNow.load()) {
                continue;
            }

            const int frameRate = frame.sample_rate();
            const int frameChannels = frame.num_channels();
            if (frameRate <= 0 || frameChannels <= 0) {
                continue;
            }

            if (!sink || activeRate != frameRate || activeChannels != frameChannels) {
                if (sink) {
                    sink->stop();
                    sink.reset();
                    sinkDevice = nullptr;
                }

                QAudioFormat format;
                format.setSampleRate(frameRate);
                format.setChannelCount(frameChannels);
                format.setSampleFormat(QAudioFormat::Int16);

                if (!outputDevice.isFormatSupported(format)) {
                    continue;
                }

                sink = std::make_unique<QAudioSink>(outputDevice, format);
                sinkDevice = sink->start();
                activeRate = frameRate;
                activeChannels = frameChannels;
            }

            if (!sinkDevice) {
                continue;
            }

            const std::vector<int16_t> &pcm = frame.data();
            const char *dataPtr = reinterpret_cast<const char *>(pcm.data());
            const qint64 totalBytes = static_cast<qint64>(pcm.size() * sizeof(int16_t));
            qint64 written = 0;
            while (written < totalBytes && !playback->stop.load()) {
                const qint64 chunkWritten = sinkDevice->write(dataPtr + written, totalBytes - written);
                if (chunkWritten > 0) {
                    written += chunkWritten;
                    continue;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        if (playback->stream) {
            playback->stream->close();
        }
        if (sink) {
            sink->stop();
        }
    });

    {
        std::lock_guard<std::mutex> lock(m_remoteAudioMutex);
        m_remotePlaybacks[trackSid] = playback;
    }
}

void NativeVoiceEngine::stopRemotePlayback(const std::string &trackSid) {
    std::shared_ptr<RemotePlayback> playback;
    {
        std::lock_guard<std::mutex> lock(m_remoteAudioMutex);
        auto it = m_remotePlaybacks.find(trackSid);
        if (it == m_remotePlaybacks.end()) {
            return;
        }
        playback = it->second;
        m_remotePlaybacks.erase(it);
    }

    if (!playback) {
        return;
    }

    playback->stop.store(true);
    if (playback->stream) {
        playback->stream->close();
    }
    if (playback->thread.joinable()) {
        playback->thread.join();
    }
}

void NativeVoiceEngine::stopAllRemotePlaybacks() {
    std::vector<std::shared_ptr<RemotePlayback>> playbacks;
    {
        std::lock_guard<std::mutex> lock(m_remoteAudioMutex);
        playbacks.reserve(m_remotePlaybacks.size());
        for (auto &entry : m_remotePlaybacks) {
            playbacks.push_back(entry.second);
        }
        m_remotePlaybacks.clear();
    }

    for (const std::shared_ptr<RemotePlayback> &playback : playbacks) {
        if (!playback) {
            continue;
        }
        playback->stop.store(true);
        if (playback->stream) {
            playback->stream->close();
        }
    }

    for (const std::shared_ptr<RemotePlayback> &playback : playbacks) {
        if (playback && playback->thread.joinable()) {
            playback->thread.join();
        }
    }
}

void NativeVoiceEngine::setConnected(bool connected) {
    if (m_connected == connected) {
        return;
    }

    m_connected = connected;
    emit connectedChanged(connected);
}

void NativeVoiceEngine::setError(const QString &error) {
    if (m_error == error) {
        return;
    }

    m_error = error;
    emit errorChanged(error);
}
