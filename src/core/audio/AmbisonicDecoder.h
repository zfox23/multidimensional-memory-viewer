#pragma once
#include <cmath>

enum class AmbisonicFormat {
    FuMa,  // Channel order: W, X, Y, Z  (Zoom H2n / SoundField native)
    AmbiX  // Channel order: W, Y, Z, X  (ACN/SN3D)
};

// First-Order Ambisonics B-format decoder with head-tracking and binaural HRTF.
// Applies a yaw/pitch/roll rotation to the B-format signals, then renders to
// stereo binaural output using a spherical head model:
//   • Virtual speakers at ±90° azimuth (natural ear positions)
//   • ITD via a 32-sample delay on the contralateral path (≈ 0.67 ms @ 48 kHz)
//   • Head shadow via a 1-pole IIR low-pass (fc ≈ 1500 Hz) on the contralateral path
class AmbisonicDecoder {
public:
    explicit AmbisonicDecoder(AmbisonicFormat format = AmbisonicFormat::FuMa);

    void setFormat(AmbisonicFormat format);
    // Orientation in radians; yaw = rotation around up axis, pitch = tilt up/down.
    void setOrientation(float yaw, float pitch, float roll = 0.0f);

    // Process one block of interleaved 4-channel input → interleaved 2-channel output.
    // frameCount is the number of sample frames (each frame = 4 input floats, 2 output floats).
    void process(const float* input4ch, float* output2ch, int frameCount) const;

private:
    AmbisonicFormat m_format;
    float m_yaw   = 0.0f;
    float m_pitch = 0.0f;
    float m_roll  = 0.0f;

    // Spherical head model HRTF state.
    // 'mutable' so process() can remain const while updating per-sample DSP state.
    static constexpr int kITDSamples = 32;   // ≈ 0.67 ms max ITD at 48 kHz
    static constexpr int kDelayLen   = 64;   // ring-buffer length; must be power-of-2 and ≥ kITDSamples

    mutable float m_delayL[kDelayLen] = {};  // right-speaker signal delayed into left ear
    mutable float m_delayR[kDelayLen] = {};  // left-speaker  signal delayed into right ear
    mutable int   m_delayPos          = 0;
    mutable float m_shadowL           = 0.0f;  // 1-pole IIR state, left-ear contralateral path
    mutable float m_shadowR           = 0.0f;  // 1-pole IIR state, right-ear contralateral path
};
