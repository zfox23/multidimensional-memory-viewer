#include "OpusReader.h"
#include <opusfile.h>
#include <QFile>

OpusReader::Result OpusReader::decode(const QString& filePath)
{
    Result result;

    int error = 0;
    OggOpusFile* of = op_open_file(filePath.toLocal8Bit().constData(), &error);
    if (!of) {
        result.error = QStringLiteral("op_open_file failed: error %1").arg(error);
        return result;
    }

    const int channels = op_channel_count(of, -1);
    if (channels < 1) {
        op_free(of);
        result.error = QStringLiteral("Invalid channel count: %1").arg(channels);
        return result;
    }

    result.channels = channels;
    result.sampleRate = 48000; // opusfile always decodes to 48 kHz

    static const int kReadSize = 5760 * 4; // 120 ms of 4-ch audio per read
    QVector<float> readBuf(kReadSize);

    while (true) {
        // op_read_float returns samples-per-channel per call, or 0 at EOF, <0 on error
        const int samplesPerChannel = op_read_float(of, readBuf.data(),
                                                     readBuf.size(), nullptr);
        if (samplesPerChannel == 0)
            break;
        if (samplesPerChannel < 0) {
            op_free(of);
            result.error = QStringLiteral("op_read_float error: %1").arg(samplesPerChannel);
            return result;
        }

        const int total = samplesPerChannel * channels;
        const int prevSize = result.pcm.size();
        result.pcm.resize(prevSize + total);
        std::copy(readBuf.cbegin(), readBuf.cbegin() + total,
                  result.pcm.begin() + prevSize);
    }

    op_free(of);
    result.ok = true;
    return result;
}
