#include "AmbisonicDecoder.h"
#include <saf.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

// ── Construction / destruction ───────────────────────────────────────────────

AmbisonicDecoder::AmbisonicDecoder(AmbisonicFormat format)
    : m_format(format)
{
    m_inStage.assign(kNCHin  * kHopSize,    0.0f);
    m_outStage.assign(kNCHout * kHopSize,   0.0f);
    m_outRing.assign(kRingFrames * kNCHout, 0.0f);
    initDecoder();
    updateRotationMatrix();
}

AmbisonicDecoder::~AmbisonicDecoder()
{
    if (m_matrixConv)
        saf_matrixConv_destroy(&m_matrixConv);
}

// ── Public API ───────────────────────────────────────────────────────────────

void AmbisonicDecoder::setFormat(AmbisonicFormat format)
{
    m_format = format;
}

void AmbisonicDecoder::setOrientation(float yaw, float pitch, float roll)
{
    m_yaw   = yaw;
    m_pitch = pitch;
    m_roll  = roll;
    m_orientationDirty = true;
}

// ── Private helpers ──────────────────────────────────────────────────────────

void AmbisonicDecoder::initDecoder()
{
    // Built-in KEMAR HRTF: 836 directions × 2 ears × 256 taps @ 48 kHz
    const int N_dirs   = __default_N_hrir_dirs;   // 836
    const int hrir_len = __default_hrir_len;       // 256
    const int hrir_fs  = __default_hrir_fs;        // 48000
    const int order    = 1;                        // first-order ambisonics
    const int nSH      = (order + 1) * (order + 1); // 4 channels
    const int fftSize  = 512;                      // must be power-of-2 ≥ hrir_len

    // Convert HRIRs → frequency-domain HRTFs
    // Output layout: (fftSize/2+1) × NUM_EARS × N_dirs
    const int N_bins = fftSize / 2 + 1;
    std::vector<float_complex> hrtfs(static_cast<size_t>(N_bins * 2 * N_dirs));
    HRIRs2HRTFs(
        const_cast<float*>(reinterpret_cast<const float*>(__default_hrirs)),
        N_dirs, hrir_len, fftSize,
        hrtfs.data());

    // Generate binaural decoder FIR filters via LS + diffuse-field EQ
    // Output layout: nSH × 2 × fftSize  (i.e. [nCHin][nCHout][length])
    std::vector<float> decFilters(static_cast<size_t>(nSH * 2 * fftSize));
    getBinauralAmbiDecoderFilters(
        hrtfs.data(),
        const_cast<float*>(reinterpret_cast<const float*>(__default_hrir_dirs_deg)),
        N_dirs,
        fftSize,
        static_cast<float>(hrir_fs),
        BINAURAL_DECODER_LSDIFFEQ,
        order,
        nullptr,  // ITDs not required for LS/LSDIFFEQ
        nullptr,  // uniform direction weights
        0,        // diffuse covariance matching: off
        1,        // max-rE weighting: on
        decFilters.data());

    // saf_matrixConv expects H in layout [nCHout × nCHin × length_h].
    // decFilters is [nCHin × nCHout × length], so transpose the first two axes.
    std::vector<float> H(static_cast<size_t>(kNCHout * kNCHin * fftSize));
    for (int ear = 0; ear < kNCHout; ++ear)
        for (int sh = 0; sh < kNCHin; ++sh)
            for (int tap = 0; tap < fftSize; ++tap)
                H[static_cast<size_t>(ear * kNCHin * fftSize + sh * fftSize + tap)] =
                    decFilters[static_cast<size_t>(sh * kNCHout * fftSize + ear * fftSize + tap)];

    saf_matrixConv_create(&m_matrixConv,
                          kHopSize,
                          H.data(),
                          fftSize,
                          kNCHin,
                          kNCHout,
                          1 /* partitioned overlap-add */);
}

void AmbisonicDecoder::updateRotationMatrix()
{
    const float cy = cosf(m_yaw),   sy = sinf(m_yaw);
    const float cp = cosf(m_pitch), sp = sinf(m_pitch);
    const float cr = cosf(m_roll),  sr = sinf(m_roll);

    // Build the 3×3 rotation matrix that matches the B-format rotation applied
    // in the previous hand-rolled decoder, preserving visual/audio alignment:
    //   Step 1: Rx(pitch)   — tilts the sound field up/down
    //   Step 2: Rz(−yaw)    — yaws opposite to head rotation so left becomes front
    //   Step 3: Rz(roll)    — rolls the field to match camera roll
    float Rpitch[3][3] = {{1, 0,  0 }, {0, cp, -sp}, {0, sp, cp}};
    float Ryaw[3][3]   = {{cy, sy, 0}, {-sy, cy, 0}, {0, 0, 1}};
    float Rroll[3][3]  = {{cr, -sr, 0}, {sr, cr, 0}, {0, 0, 1}};

    // Combined R = Rroll × Ryaw × Rpitch
    float temp[3][3] = {};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                temp[i][j] += Ryaw[i][k] * Rpitch[k][j];

    float R[3][3] = {};
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                R[i][j] += Rroll[i][k] * temp[k][j];

    // getSHrotMtxReal produces a (L+1)² × (L+1)² = 4×4 matrix for L=1 (FOA).
    // The result is applied to ACN-ordered signals: outSig = RotMtx × inSig.
    getSHrotMtxReal(R, m_rotMtx, 1 /* FOA order */);
}

// ── Audio processing ─────────────────────────────────────────────────────────

void AmbisonicDecoder::process(const float* in4ch, float* out2ch, int frameCount)
{
    std::fill(out2ch, out2ch + frameCount * 2, 0.0f);

    if (!m_matrixConv)
        return;

    if (m_orientationDirty) {
        updateRotationMatrix();
        m_orientationDirty = false;
    }

    // ── Stage input and process complete hops ────────────────────────────────
    int inOffset = 0;
    while (inOffset < frameCount) {
        const int toAdd = std::min(frameCount - inOffset, kHopSize - m_stageCount);

        for (int i = 0; i < toAdd; ++i) {
            const float* frame = in4ch + (inOffset + i) * 4;
            float W, Y, Z, X;
            if (m_format == AmbisonicFormat::FuMa) {
                // FuMa W carries a 1/√2 normalisation factor → undo to SN3D
                W = frame[0] * 1.41421356f;
                X = frame[1]; Y = frame[2]; Z = frame[3];
            } else {
                // AmbiX ACN/SN3D: [W, Y, Z, X]
                W = frame[0]; Y = frame[1]; Z = frame[2]; X = frame[3];
            }

            // Assemble ACN-ordered vector [W, Y, Z, X] and apply 4×4 SH rotation
            const float acn[4] = {W, Y, Z, X};
            const int   slot   = m_stageCount + i;
            for (int ch = 0; ch < kNCHin; ++ch) {
                float val = 0.0f;
                for (int k = 0; k < kNCHin; ++k)
                    val += m_rotMtx[ch * kNCHin + k] * acn[k];
                m_inStage[static_cast<size_t>(ch * kHopSize + slot)] = val;
            }
        }

        m_stageCount += toAdd;
        inOffset     += toAdd;

        if (m_stageCount == kHopSize) {
            // Run SAF partitioned convolution: 4ch planar in → 2ch planar out
            saf_matrixConv_apply(m_matrixConv,
                                 m_inStage.data(),
                                 m_outStage.data());

            // Interleave SAF's planar output into the ring buffer
            for (int i = 0; i < kHopSize; ++i) {
                const int pos = (m_ringWrite + i) % kRingFrames;
                m_outRing[static_cast<size_t>(pos * kNCHout + 0)] =
                    m_outStage[static_cast<size_t>(0 * kHopSize + i)];
                m_outRing[static_cast<size_t>(pos * kNCHout + 1)] =
                    m_outStage[static_cast<size_t>(1 * kHopSize + i)];
            }
            m_ringWrite  = (m_ringWrite + kHopSize) % kRingFrames;
            m_ringCount += kHopSize;
            m_stageCount = 0;
        }
    }

    // ── Drain ring buffer to output ──────────────────────────────────────────
    const int toDrain = std::min(m_ringCount, frameCount);
    for (int i = 0; i < toDrain; ++i) {
        const int pos = (m_ringRead + i) % kRingFrames;
        out2ch[i * 2 + 0] = m_outRing[static_cast<size_t>(pos * kNCHout + 0)];
        out2ch[i * 2 + 1] = m_outRing[static_cast<size_t>(pos * kNCHout + 1)];
    }
    m_ringRead  = (m_ringRead + toDrain) % kRingFrames;
    m_ringCount -= toDrain;
}
