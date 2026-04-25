#pragma once
#include "AmbisonicDecoder.h"
#include <QObject>
#include <QIODevice>
#include <QAudioSink>
#include <QAudioFormat>
#include <QVector>
#include <atomic>

// QIODevice that reads from a looping float32 PCM buffer, applying
// an ambisonic decode on each block to produce stereo output.
class AmbisonicAudioDevice : public QIODevice {
    Q_OBJECT
public:
    explicit AmbisonicAudioDevice(QObject* parent = nullptr);

    void setData(QVector<float> pcm, int channels);
    void setDecoder(AmbisonicDecoder* decoder);
    void resetPlayback();

    // Thread-safe: readable from any thread while audio thread writes.
    float positionRatio() const;
    qint64 totalFrames() const { return m_totalFrames.load(std::memory_order_relaxed); }

    // Must return true so QIODevice::read() calls readData() directly
    // instead of checking bytesAvailable() (which returns 0 for custom devices).
    bool isSequential() const override { return true; }

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QVector<float> m_pcm;
    int m_channels = 4;
    std::atomic<qint64> m_framePos{0};
    std::atomic<qint64> m_totalFrames{0};
    AmbisonicDecoder* m_decoder = nullptr;
};

class AudioEngine : public QObject {
    Q_OBJECT
public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;

    bool loadFile(const QString& filePath);
    void play();
    void stop();

    void setMuted(bool muted);
    bool isMuted() const;

    float audioPosition() const;

    // Called from the UI thread whenever the camera orientation changes.
    void setOrientation(float yaw, float pitch, float roll = 0.0f);

    // Compute a normalized amplitude envelope from a decoded PCM buffer.
    // Returns numBuckets values in [0, 1] representing peak amplitude per window.
    // Uses the W (omnidirectional) channel for the best mono representation.
    static QVector<float> computeWaveform(const QVector<float>& pcm,
                                          int channels,
                                          int numBuckets = 200);

signals:
    void mutedChanged(bool muted);
    void loadError(const QString& message);
    void positionChanged(float position);   // fired ~20×/sec

private:
    void setupSink();

    AmbisonicDecoder m_decoder;
    AmbisonicAudioDevice m_device;
    QAudioSink* m_sink = nullptr;
    bool m_muted = false;
    class QTimer* m_positionTimer = nullptr;
};
