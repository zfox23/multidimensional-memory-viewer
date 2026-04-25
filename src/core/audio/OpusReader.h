#pragma once
#include <QVector>
#include <QString>

// Decodes a multi-channel .opus file to interleaved float32 PCM at 48 kHz.
class OpusReader {
public:
    struct Result {
        QVector<float> pcm;   // interleaved samples, channels × frames
        int channels = 0;
        int sampleRate = 48000;
        bool ok = false;
        QString error;
    };

    static Result decode(const QString& filePath);
};
