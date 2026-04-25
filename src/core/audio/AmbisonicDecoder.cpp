#include "AmbisonicDecoder.h"
#include <cstring>

AmbisonicDecoder::AmbisonicDecoder(AmbisonicFormat format)
    : m_format(format)
{
    std::memset(m_delayL, 0, sizeof(m_delayL));
    std::memset(m_delayR, 0, sizeof(m_delayR));
}

void AmbisonicDecoder::setFormat(AmbisonicFormat format)
{
    m_format = format;
}

void AmbisonicDecoder::setOrientation(float yaw, float pitch, float roll)
{
    m_yaw   = yaw;
    m_pitch = pitch;
    m_roll  = roll;
}

void AmbisonicDecoder::process(const float* in, float* out, int frameCount) const
{
    // ── Spherical head model HRTF parameters ────────────────────────────────
    //
    // Head radius a = 0.0875 m, speed of sound c = 343 m/s.
    //
    // ITD (Woodworth formula, max at ±90° azimuth):
    //   τ_max = (a/c) × (1 + π/2) ≈ 0.655 ms ≈ 32 samples @ 48 kHz  → kITDSamples
    //
    // Head shadow (first-order IIR low-pass, cutoff fc ≈ c/(2πa) ≈ 623 Hz
    // theoretical; perceptually effective cutoff is closer to 1500 Hz):
    //   α = 1 − exp(−2π·fc/fs)
    static const float kShadowA =
        1.0f - std::exp(-2.0f * 3.14159265f * 1500.0f / 48000.0f);

    // FOA decode gains (max-rE criterion, horizontal plane, 2 virtual speakers at ±90°).
    //   kW:     omnidirectional channel weight
    //   kD:     directional channel weight (Y = left-right)
    //   kFront: front-back channel weight (X added equally to both ears;
    //           cos(±90°) = 0 so X is orthogonal to the ±90° speaker pair,
    //           but contributes important front-back presence)
    static const float kW     = 0.50f;
    static const float kD     = 0.50f;
    static const float kFront = 0.25f;

    // HRTF path gains:
    //   kDirect: ipsilateral path  (speaker closest to the ear)
    //   kContra: contralateral path (speaker on the far side, through head shadow)
    static const float kDirect = 1.00f;
    static const float kContra = 0.50f;

    // ── Rotation angles ──────────────────────────────────────────────────────
    const float cy = std::cos(m_yaw),   sy = std::sin(m_yaw);
    const float cp = std::cos(m_pitch), sp = std::sin(m_pitch);
    const float cr = std::cos(m_roll),  sr = std::sin(m_roll);

    for (int i = 0; i < frameCount; ++i) {
        const float* frame = in + i * 4;
        float W, X, Y, Z;

        if (m_format == AmbisonicFormat::FuMa) {
            // FuMa: [W, X, Y, Z]; W carries a 1/√2 normalization factor
            W = frame[0] * 1.41421356f; // undo FuMa 1/√2 → SN3D
            X = frame[1];
            Y = frame[2];
            Z = frame[3];
        } else {
            // AmbiX ACN/SN3D: [W, Y, Z, X]
            W = frame[0];
            Y = frame[1];
            Z = frame[2];
            X = frame[3];
        }

        // ── B-format rotation (pitch first, then yaw, then roll) ─────────────
        // This order — Ry(yaw) × Rx(pitch) — matches the fragment shader so the
        // sound field rotates identically to the visual view.
        float Xr = X;
        float Yr =  Y * cp - Z * sp;
        float Zr =  Y * sp + Z * cp;

        float Xr2 =  Xr * cy + Yr * sy;
        float Yr2 = -Xr * sy + Yr * cy;
        Xr = Xr2;
        Yr = Yr2;

        Xr2       = Xr * cr - Yr * sr;
        float Yr3 = Xr * sr + Yr * cr;
        Xr = Xr2;
        Yr = Yr3;
        (void)Zr;

        // ── Binaural decode ──────────────────────────────────────────────────
        //
        // Two virtual speakers at ±90° azimuth (ear positions):
        //   spkL = signal arriving from the left  (ipsilateral for the left ear)
        //   spkR = signal arriving from the right (ipsilateral for the right ear)
        //
        // The front-back component (Xr) is orthogonal to the ±90° speaker axis
        // (cos(±90°) = 0) so it contributes equally to both ears as a "mono"
        // presence signal — providing front-back audibility without false panning.
        const float spkL  = kW * W + kD * Yr;
        const float spkR  = kW * W - kD * Yr;
        const float front = kFront * Xr;

        // ── HRTF: contralateral path = ITD delay + head-shadow low-pass ──────
        //
        // The contralateral (far-side) signal reaches the ear:
        //   1. kITDSamples later (time delay around the head)
        //   2. Low-pass filtered by the head shadow
        //
        // Store each speaker's signal in a ring buffer so we can read it back
        // kITDSamples later for the opposite ear.
        m_delayL[m_delayPos] = spkR;   // right speaker → delayed → left ear
        m_delayR[m_delayPos] = spkL;   // left  speaker → delayed → right ear

        const int rd = (m_delayPos - kITDSamples + kDelayLen) & (kDelayLen - 1);
        const float contraForL = m_delayL[rd];
        const float contraForR = m_delayR[rd];
        m_delayPos = (m_delayPos + 1) & (kDelayLen - 1);

        // 1-pole IIR low-pass models the head shadow on the contralateral path.
        // High frequencies (> ~1500 Hz) are progressively attenuated, giving
        // the contralateral ear a "muffled" character — the dominant HRTF cue
        // for lateral localization at high frequencies.
        m_shadowL += kShadowA * (contraForL - m_shadowL);
        m_shadowR += kShadowA * (contraForR - m_shadowR);

        // Sum ipsilateral (direct, full bandwidth) + contralateral (delayed, shadowed).
        out[i * 2 + 0] = kDirect * spkL + kContra * m_shadowL + front;
        out[i * 2 + 1] = kDirect * spkR + kContra * m_shadowR + front;
    }
}
