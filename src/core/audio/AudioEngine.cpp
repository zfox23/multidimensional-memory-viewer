#include "AudioEngine.h"
#include "OpusReader.h"
#include <QAudioDevice>
#include <QMediaDevices>
#include <cstring>

// ── AmbisonicAudioDevice ─────────────────────────────────────────────────────

AmbisonicAudioDevice::AmbisonicAudioDevice(QObject* parent)
    : QIODevice(parent)
{}

void AmbisonicAudioDevice::setData(QVector<float> pcm, int channels)
{
    m_pcm = std::move(pcm);
    m_channels = channels;
    m_totalFrames = (m_channels > 0) ? m_pcm.size() / m_channels : 0;
    m_framePos = 0;
}

void AmbisonicAudioDevice::setDecoder(AmbisonicDecoder* decoder)
{
    m_decoder = decoder;
}

void AmbisonicAudioDevice::resetPlayback()
{
    m_framePos = 0;
}

qint64 AmbisonicAudioDevice::readData(char* data, qint64 maxSize)
{
    if (m_pcm.isEmpty() || m_totalFrames == 0)
        return 0;

    // Output is stereo float32 → 8 bytes per frame
    const qint64 framesRequested = maxSize / (2 * sizeof(float));
    if (framesRequested == 0)
        return 0;

    auto* out = reinterpret_cast<float*>(data);
    qint64 framesWritten = 0;

    while (framesWritten < framesRequested) {
        const qint64 remaining = m_totalFrames - m_framePos;
        const qint64 batch = std::min(framesRequested - framesWritten, remaining);

        const float* src = m_pcm.constData() + m_framePos * m_channels;
        float* dst = out + framesWritten * 2;

        if (m_decoder && m_channels == 4) {
            m_decoder->process(src, dst, static_cast<int>(batch));
        } else {
            // Fallback: mix all input channels to stereo equally
            for (qint64 f = 0; f < batch; ++f) {
                float sumL = 0, sumR = 0;
                for (int c = 0; c < m_channels; ++c) {
                    const float s = src[f * m_channels + c];
                    (c % 2 == 0 ? sumL : sumR) += s;
                }
                const float scale = (m_channels > 0) ? 1.0f / (m_channels / 2.0f) : 1.0f;
                dst[f * 2 + 0] = sumL * scale;
                dst[f * 2 + 1] = sumR * scale;
            }
        }

        framesWritten += batch;
        m_framePos += batch;

        // Loop seamlessly back to the start
        if (m_framePos >= m_totalFrames)
            m_framePos = 0;
    }

    return framesWritten * 2 * sizeof(float);
}

// ── AudioEngine ──────────────────────────────────────────────────────────────

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
{
    m_device.setDecoder(&m_decoder);
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
        emit loadError(result.error);
        return false;
    }

    m_device.setData(std::move(result.pcm), result.channels);
    m_decoder.setFormat(result.channels == 4 ? AmbisonicFormat::FuMa : AmbisonicFormat::FuMa);
    setupSink();
    return true;
}

void AudioEngine::setupSink()
{
    QAudioFormat fmt;
    fmt.setSampleRate(48000);
    fmt.setChannelCount(2);
    fmt.setSampleFormat(QAudioFormat::Float);

    delete m_sink;
    m_sink = new QAudioSink(QMediaDevices::defaultAudioOutput(), fmt, this);
    m_sink->setVolume(m_muted ? 0.0 : 1.0);
}

void AudioEngine::play()
{
    if (!m_sink)
        return;
    m_device.resetPlayback();
    if (!m_device.isOpen())
        m_device.open(QIODevice::ReadOnly);
    m_sink->start(&m_device);
}

void AudioEngine::stop()
{
    if (m_sink)
        m_sink->stop();
    if (m_device.isOpen())
        m_device.close();
}

void AudioEngine::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    if (m_sink)
        m_sink->setVolume(muted ? 0.0 : 1.0);
    emit mutedChanged(muted);
}

bool AudioEngine::isMuted() const
{
    return m_muted;
}

void AudioEngine::setOrientation(float yaw, float pitch, float roll)
{
    m_decoder.setOrientation(yaw, pitch, roll);
}
