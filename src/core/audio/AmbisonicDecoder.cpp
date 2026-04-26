#include "AmbisonicDecoder.h"
#include <ambi_bin.h>
#include <algorithm>
#include <cstring>

static constexpr float kRadToDeg = 57.29577951308232f;  // 180/pi

AmbisonicDecoder::AmbisonicDecoder(AmbisonicFormat format)
    : m_format(format)
{
    ambi_bin_create(&m_hAmbi);
    ambi_bin_setInputOrderPreset(m_hAmbi, SH_ORDER_FIRST);
    ambi_bin_setEnableRotation(m_hAmbi, 1);
    applyFormat();
    ambi_bin_initCodec(m_hAmbi);
    ambi_bin_init(m_hAmbi, 48000);

    m_frameSize = ambi_bin_getFrameSize();

    m_inBuf.assign(kNCHin  * m_frameSize, 0.0f);
    m_outBuf.assign(kNCHout * m_frameSize, 0.0f);
    m_outRing.assign(kRingFrames * kNCHout, 0.0f);

    for (int i = 0; i < kNCHin;  ++i) m_inPtrs[i]  = m_inBuf.data()  + i * m_frameSize;
    for (int i = 0; i < kNCHout; ++i) m_outPtrs[i] = m_outBuf.data() + i * m_frameSize;
}

AmbisonicDecoder::~AmbisonicDecoder()
{
    ambi_bin_destroy(&m_hAmbi);
}

void AmbisonicDecoder::setFormat(AmbisonicFormat format)
{
    if (m_format == format) return;
    m_format = format;
    applyFormat();
    ambi_bin_initCodec(m_hAmbi);
}

void AmbisonicDecoder::setOrientation(float yaw, float pitch, float roll)
{
    ambi_bin_setYaw  (m_hAmbi, yaw   * kRadToDeg);
    ambi_bin_setPitch(m_hAmbi, pitch * kRadToDeg);
    ambi_bin_setRoll (m_hAmbi, roll  * kRadToDeg);
}

void AmbisonicDecoder::applyFormat()
{
    if (m_format == AmbisonicFormat::AmbiX) {
        ambi_bin_setChOrder (m_hAmbi, CH_ACN);
        ambi_bin_setNormType(m_hAmbi, NORM_SN3D);
    } else {
        ambi_bin_setChOrder (m_hAmbi, CH_FUMA);
        ambi_bin_setNormType(m_hAmbi, NORM_FUMA);
    }
}

void AmbisonicDecoder::process(const float* in4ch, float* out2ch, int frameCount)
{
    std::fill(out2ch, out2ch + frameCount * 2, 0.0f);

    int inOffset = 0;
    while (inOffset < frameCount) {
        const int toAdd = std::min(frameCount - inOffset, m_frameSize - m_inCount);

        for (int i = 0; i < toAdd; ++i) {
            const float* frame = in4ch + (inOffset + i) * kNCHin;
            for (int ch = 0; ch < kNCHin; ++ch)
                m_inPtrs[ch][m_inCount + i] = frame[ch];
        }

        m_inCount += toAdd;
        inOffset  += toAdd;

        if (m_inCount == m_frameSize) {
            ambi_bin_process(m_hAmbi,
                             (const float *const *)m_inPtrs,
                             (float *const *)m_outPtrs,
                             kNCHin, kNCHout, m_frameSize);

            for (int i = 0; i < m_frameSize; ++i) {
                const int pos = (m_ringWrite + i) % kRingFrames;
                m_outRing[pos * kNCHout + 0] = m_outPtrs[0][i];
                m_outRing[pos * kNCHout + 1] = m_outPtrs[1][i];
            }
            m_ringWrite  = (m_ringWrite + m_frameSize) % kRingFrames;
            m_ringCount += m_frameSize;
            m_inCount    = 0;
        }
    }

    const int toDrain = std::min(m_ringCount, frameCount);
    for (int i = 0; i < toDrain; ++i) {
        const int pos = (m_ringRead + i) % kRingFrames;
        out2ch[i * 2 + 0] = m_outRing[pos * kNCHout + 0];
        out2ch[i * 2 + 1] = m_outRing[pos * kNCHout + 1];
    }
    m_ringRead  = (m_ringRead + toDrain) % kRingFrames;
    m_ringCount -= toDrain;
}
