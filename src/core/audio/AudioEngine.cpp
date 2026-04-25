#include "AudioEngine.h"
#include "OpusReader.h"
#include <QAudioDevice>
#include <QMediaDevices>
#include <QTimer>
#include <algorithm>
#include <cstring>

// ── AmbisonicAudioDevice ─────────────────────────────────────────────────────

AmbisonicAudioDevice::AmbisonicAudioDevice(QObject* parent)
    : QIODevice(parent)
{}

void AmbisonicAudioDevice::setData(QVector<float> pcm, int channels)
{
    m_pcm = std::move(pcm);
    m_channels = channels;
    const qint64 frames = (channels > 0) ? m_pcm.size() / channels : 0;
    m_totalFrames.store(frames, std::memory_order_release);
    m_framePos.store(0, std::memory_order_release);
}

void AmbisonicAudioDevice::setDecoder(AmbisonicDecoder* decoder)
{
    m_decoder = decoder;
}

void AmbisonicAudioDevice::resetPlayback()
{
    m_framePos.store(0, std::memory_order_release);
}

float AmbisonicAudioDevice::positionRatio() const
{
    const qint64 total = m_totalFrames.load(std::memory_order_acquire);
    if (total <= 0) return 0.0f;
    return static_cast<float>(m_framePos.load(std::memory_order_acquire)) / total;
}

qint64 AmbisonicAudioDevice::readData(char* data, qint64 maxSize)
{
    const qint64 total = m_totalFrames.load(std::memory_order_relaxed);
    if (m_pcm.isEmpty() || total == 0)
        return 0;

    // Output: stereo float32 → 8 bytes per frame
    const qint64 framesRequested = maxSize / (2 * sizeof(float));
    if (framesRequested == 0)
        return 0;

    auto* out = reinterpret_cast<float*>(data);
    qint64 framesWritten = 0;
    qint64 pos = m_framePos.load(std::memory_order_relaxed);

    while (framesWritten < framesRequested) {
        const qint64 remaining = total - pos;
        const qint64 batch = std::min(framesRequested - framesWritten, remaining);

        const float* src = m_pcm.constData() + pos * m_channels;
        float* dst = out + framesWritten * 2;

        if (m_decoder && m_channels == 4) {
            m_decoder->process(src, dst, static_cast<int>(batch));
        } else {
            // Fallback: fold all input channels into stereo equally
            for (qint64 f = 0; f < batch; ++f) {
                float sumL = 0, sumR = 0;
                for (int c = 0; c < m_channels; ++c) {
                    const float s = src[f * m_channels + c];
                    (c % 2 == 0 ? sumL : sumR) += s;
                }
                const float scale = (m_channels > 1) ? 2.0f / m_channels : 1.0f;
                dst[f * 2 + 0] = sumL * scale;
                dst[f * 2 + 1] = sumR * scale;
            }
        }

        framesWritten += batch;
        pos += batch;
        if (pos >= total)
            pos = 0;   // seamless loop
    }

    m_framePos.store(pos, std::memory_order_release);
    return framesWritten * 2 * static_cast<qint64>(sizeof(float));
}

// ── AudioEngine ──────────────────────────────────────────────────────────────

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
{
    m_device.setDecoder(&m_decoder);

    m_positionTimer = new QTimer(this);
    m_positionTimer->setInterval(50);   // ~20 Hz position updates
    connect(m_positionTimer, &QTimer::timeout, this, [this]() {
        emit positionChanged(m_device.positionRatio());
    });
}

AudioEngine::~AudioEngine()
{
    stop();
}

bool AudioEngine::loadFile(const QString& filePath)
{
    stop();
    if (filePath.isEmpty())
        return false;

    auto result = OpusReader::decode(filePath);
    if (!result.ok) {
        qWarning("AudioEngine: failed to decode '%s': %s",
                 qPrintable(filePath), qPrintable(result.error));
        emit loadError(result.error);
        return false;
    }

    qDebug("AudioEngine: loaded %d-ch, %d frames from '%s'",
           result.channels, result.pcm.size() / qMax(1, result.channels),
           qPrintable(filePath));

    m_device.setData(std::move(result.pcm), result.channels);
    m_decoder.setFormat(result.channels == 4 ? AmbisonicFormat::FuMa : AmbisonicFormat::FuMa);
    setupSink();
    return true;
}

void AudioEngine::setupSink()
{
    delete m_sink;
    m_sink = nullptr;

    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        qWarning("AudioEngine: no default audio output device found");
        return;
    }

    QAudioFormat fmt;
    fmt.setSampleRate(48000);
    fmt.setChannelCount(2);
    fmt.setSampleFormat(QAudioFormat::Float);

    if (!device.isFormatSupported(fmt)) {
        qWarning("AudioEngine: Float32 format not supported, trying Int16");
        fmt.setSampleFormat(QAudioFormat::Int16);
        if (!device.isFormatSupported(fmt)) {
            qWarning("AudioEngine: Int16 format not supported either; trying preferred format");
            fmt = device.preferredFormat();
            fmt.setChannelCount(2);
        }
    }

    m_sink = new QAudioSink(device, fmt, this);
    m_sink->setVolume(m_muted ? 0.0 : 1.0);

    qDebug("AudioEngine: sink created — rate=%d ch=%d fmt=%d",
           fmt.sampleRate(), fmt.channelCount(), (int)fmt.sampleFormat());
}

void AudioEngine::play()
{
    if (!m_sink) return;

    m_device.resetPlayback();

    // Clean up any previous push timer.
    if (m_audioTimer) {
        m_audioTimer->stop();
        delete m_audioTimer;
        m_audioTimer = nullptr;
    }
    m_pushDevice = nullptr;

    // Push mode: the sink gives us a writable QIODevice; we feed it from a timer.
    // This is more reliable than pull mode on macOS CoreAudio.
    m_pushDevice = m_sink->start();

    if (!m_pushDevice || m_sink->error() != QAudio::NoError) {
        qWarning("AudioEngine: sink start failed, error=%d", (int)m_sink->error());
        return;
    }

    qDebug("AudioEngine: playback started (push mode), state=%d", (int)m_sink->state());

    // Prime the buffer immediately, then top it up every 20 ms.
    pushAudio();

    m_audioTimer = new QTimer(this);
    m_audioTimer->setInterval(20);
    connect(m_audioTimer, &QTimer::timeout, this, &AudioEngine::pushAudio);
    m_audioTimer->start();

    m_positionTimer->start();
}

void AudioEngine::pushAudio()
{
    if (!m_pushDevice || !m_sink) return;
    const qint64 bytesFree = m_sink->bytesFree();
    if (bytesFree <= 0) return;

    QByteArray buf(bytesFree, '\0');
    const qint64 written = m_device.readData(buf.data(), bytesFree);
    if (written > 0)
        m_pushDevice->write(buf.constData(), written);
}

void AudioEngine::stop()
{
    m_positionTimer->stop();
    if (m_audioTimer) {
        m_audioTimer->stop();
        delete m_audioTimer;
        m_audioTimer = nullptr;
    }
    m_pushDevice = nullptr;
    if (m_sink) m_sink->stop();
}

void AudioEngine::setMuted(bool muted)
{
    if (m_muted == muted) return;
    m_muted = muted;
    if (m_sink) m_sink->setVolume(muted ? 0.0 : 1.0);
    emit mutedChanged(muted);
}

bool AudioEngine::isMuted() const { return m_muted; }

float AudioEngine::audioPosition() const { return m_device.positionRatio(); }

void AudioEngine::setOrientation(float yaw, float pitch, float roll)
{
    m_decoder.setOrientation(yaw, pitch, roll);
}

QVector<float> AudioEngine::computeWaveform(const QVector<float>& pcm,
                                             int channels,
                                             int numBuckets)
{
    if (pcm.isEmpty() || channels < 1 || numBuckets < 1)
        return {};

    const qint64 totalFrames = pcm.size() / channels;
    QVector<float> waveform(numBuckets, 0.0f);

    for (int b = 0; b < numBuckets; ++b) {
        const qint64 start = (static_cast<qint64>(b) * totalFrames) / numBuckets;
        const qint64 end   = (static_cast<qint64>(b + 1) * totalFrames) / numBuckets;
        float peak = 0.0f;
        for (qint64 f = start; f < end; ++f) {
            // Use channel 0 (W in FuMa / omnidirectional) for the envelope
            peak = std::max(peak, std::abs(pcm[f * channels]));
        }
        waveform[b] = peak;
    }

    // Normalize so the loudest bucket reaches 1.0
    const float maxVal = *std::max_element(waveform.begin(), waveform.end());
    if (maxVal > 1e-6f) {
        for (float& v : waveform)
            v /= maxVal;
    }

    return waveform;
}
