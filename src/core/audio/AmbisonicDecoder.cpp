#include "AmbisonicDecoder.h"

AmbisonicDecoder::AmbisonicDecoder(AmbisonicFormat format)
    : m_format(format)
{}

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
    // Virtual speaker azimuths: left at +30°, right at -30°
    static const float kLAz  = 30.0f * (3.14159265f / 180.0f);
    static const float kRAz  = -30.0f * (3.14159265f / 180.0f);
    // Max-rE decode gains for a 2-speaker stereo array (first-order)
    static const float kW = 0.5f;
    static const float kD = 0.5f; // directional component weight

    const float cy = std::cos(m_yaw),   sy = std::sin(m_yaw);
    const float cp = std::cos(m_pitch), sp = std::sin(m_pitch);
    const float cr = std::cos(m_roll),  sr = std::sin(m_roll);

    // Decode gains for each virtual speaker (W + directional)
    const float leftX  = std::cos(kLAz);
    const float leftY  = std::sin(kLAz);
    const float rightX = std::cos(kRAz);
    const float rightY = std::sin(kRAz);

    for (int i = 0; i < frameCount; ++i) {
        const float* frame = in + i * 4;
        float W, X, Y, Z;

        if (m_format == AmbisonicFormat::FuMa) {
            // FuMa: [W, X, Y, Z]; W has a 1/sqrt(2) normalization factor
            W = frame[0] * 1.41421356f; // undo FuMa 1/sqrt(2) weighting → SN3D
            X = frame[1];
            Y = frame[2];
            Z = frame[3];
        } else {
            // AmbiX ACN: [W(ACN0), Y(ACN1), Z(ACN2), X(ACN3)] — already SN3D
            W = frame[0];
            Y = frame[1];
            Z = frame[2];
            X = frame[3];
        }

        // Apply yaw rotation (around Y/up axis)
        float Xr =  X * cy + Y * sy;
        float Yr = -X * sy + Y * cy;
        float Zr = Z;

        // Apply pitch rotation (around X/right axis)
        float Yr2 =  Yr * cp - Zr * sp;
        float Zr2 =  Yr * sp + Zr * cp;
        Yr = Yr2;
        Zr = Zr2;

        // Apply roll rotation (around Z/forward axis) — kept for VR completeness
        float Xr2 = Xr * cr - Yr * sr;
        float Yr3 = Xr * sr + Yr * cr;
        Xr = Xr2;
        Yr = Yr3;
        (void)Zr; // Z used for elevation; not needed for 2D stereo decode

        // Stereo decode: sum of omnidirectional (W) and directional (X/Y) components
        out[i * 2 + 0] = kW * W + kD * (Xr * leftX  + Yr * leftY);   // left
        out[i * 2 + 1] = kW * W + kD * (Xr * rightX + Yr * rightY);  // right
    }
}
