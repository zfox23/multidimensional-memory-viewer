#pragma once
#include <cmath>

enum class AmbisonicFormat {
    FuMa,  // Channel order: W, X, Y, Z  (Zoom H2n / SoundField native)
    AmbiX  // Channel order: W, Y, Z, X  (ACN/SN3D)
};

// First-Order Ambisonics B-format decoder with head-tracking.
// Applies a yaw/pitch/roll rotation to the B-format signals, then decodes
// to stereo using a virtual speaker pair at ±30° azimuth.
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
};
