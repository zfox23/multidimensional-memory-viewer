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

protected:
    qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char*, qint64) override { return -1; }

private:
    QVector<float> m_pcm;
    int m_channels = 4;
    qint64 m_framePos = 0;   // current read head in frames
    qint64 m_totalFrames = 0;
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

    // Called from the UI thread whenever the camera orientation changes.
    void setOrientation(float yaw, float pitch, float roll = 0.0f);

signals:
    void mutedChanged(bool muted);
    void loadError(const QString& message);

private:
    void setupSink();

    AmbisonicDecoder m_decoder;
    AmbisonicAudioDevice m_device;
    QAudioSink* m_sink = nullptr;
    bool m_muted = false;
};
