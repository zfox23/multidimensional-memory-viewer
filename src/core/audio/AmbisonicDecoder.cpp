#include "AmbisonicDecoder.h"
#include <ambi_bin.h>
#include <algorithm>
#include <cstring>
#include <QDebug>

static constexpr float kRadToDeg = 57.29577951308232f;  // 180/pi

AmbisonicDecoder::AmbisonicDecoder(AmbisonicFormat format)
    : m_format(format)
{
    ambi_bin_create(&m_hAmbi);
    ambi_bin_setInputOrderPreset(m_hAmbi, SH_ORDER_FIRST);
    ambi_bin_setEnableRotation(m_hAmbi, 1);
    applyFormat();

    // ambi_bin_init must come before ambi_bin_initCodec: init resets
    // codecStatus=NOT_INITIALISED on its first call (firstInit flag), so
    // any initCodec called before it will be undone.
    ambi_bin_init(m_hAmbi, 48000);
    ambi_bin_initCodec(m_hAmbi);

    m_frameSize = ambi_bin_getFrameSize();

    m_inBuf.assign(kNCHin  * m_frameSize, 0.0f);
    m_outBuf.assign(kNCHout * m_frameSize, 0.0f);

    for (int i = 0; i < kNCHin;  ++i) m_inPtrs[i]  = m_inBuf.data()  + i * m_frameSize;
    for (int i = 0; i < kNCHout; ++i) m_outPtrs[i] = m_outBuf.data() + i * m_frameSize;

    qDebug("AmbisonicDecoder: created — frameSize=%d processingDelay=%d codecStatus=%d (0=OK)",
           m_frameSize, ambi_bin_getProcessingDelay(),
           (int)ambi_bin_getCodecStatus(m_hAmbi));
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
    qDebug("AmbisonicDecoder: format changed — codecStatus=%d",
           (int)ambi_bin_getCodecStatus(m_hAmbi));
}

void AmbisonicDecoder::setOrientation(float yaw, float pitch, float roll)
{
    ambi_bin_setYaw  (m_hAmbi, yaw   * kRadToDeg);
    ambi_bin_setPitch(m_hAmbi, pitch * kRadToDeg);
    ambi_bin_setRoll (m_hAmbi, roll  * kRadToDeg);
}

void AmbisonicDecoder::reset()
{
    m_inCount         = 0;
    m_fadeInRemaining = kFadeInFrames;
    std::fill(m_inBuf.begin(), m_inBuf.end(), 0.0f);
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

    int outOffset = 0;
    int inOffset  = 0;

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
            m_inCount = 0;

            // Write directly to output buffer — always fits since:
            //   outOffset + m_frameSize ≤ inOffset ≤ frameCount
            for (int i = 0; i < m_frameSize; ++i) {
                out2ch[(outOffset + i) * 2 + 0] = m_outPtrs[0][i];
                out2ch[(outOffset + i) * 2 + 1] = m_outPtrs[1][i];
            }
            outOffset += m_frameSize;
        }
    }

    // Apply fade-in ramp to mask STFT onset transient after reset
    if (m_fadeInRemaining > 0) {
        const int toFade = std::min(m_fadeInRemaining, outOffset);
        const int rampStart = kFadeInFrames - m_fadeInRemaining;
        for (int i = 0; i < toFade; ++i) {
            const float gain = static_cast<float>(rampStart + i) / kFadeInFrames;
            out2ch[i * 2 + 0] *= gain;
            out2ch[i * 2 + 1] *= gain;
        }
        m_fadeInRemaining -= toFade;
    }
}
