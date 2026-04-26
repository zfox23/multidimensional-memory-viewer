#pragma once
#include <vector>

enum class AmbisonicFormat {
    FuMa,  // Channel order: W, X, Y, Z  (Zoom H2n / SoundField native)
    AmbiX  // Channel order: W, Y, Z, X  (ACN/SN3D)
};

// First-Order Ambisonics B-format → stereo binaural decoder using the
// Spatial Audio Framework (SAF) with the built-in KEMAR HRTF dataset.
//
// Decoding pipeline per audio block:
//   1. Convert input from FuMa or AmbiX to ACN/SN3D
//   2. Apply a 4×4 SH rotation matrix derived from the current head orientation
//   3. Block-convolve the rotated 4-channel signal with a binaural decoder
//      filter bank (SAF MagLS method, KEMAR HRTF @ 48 kHz, 512-tap FIR)
//   4. Output interleaved stereo
class AmbisonicDecoder {
public:
    explicit AmbisonicDecoder(AmbisonicFormat format = AmbisonicFormat::FuMa);
    ~AmbisonicDecoder();

    void setFormat(AmbisonicFormat format);
    // Orientation in radians; yaw = rotation around up axis, pitch = tilt up/down.
    void setOrientation(float yaw, float pitch, float roll = 0.0f);

    // Process one block of interleaved 4-channel input → interleaved 2-channel output.
    // frameCount is the number of sample frames (4 input floats / 2 output floats each).
    void process(const float* input4ch, float* output2ch, int frameCount);

private:
    void initDecoder();
    void updateRotationMatrix();

    AmbisonicFormat m_format;
    float m_yaw   = 0.0f;
    float m_pitch = 0.0f;
    float m_roll  = 0.0f;
    bool  m_orientationDirty = true;

    float m_rotMtx[4 * 4] = {};  // 4×4 SH rotation matrix, ACN order

    void* m_matrixConv = nullptr;  // saf_matrixConv handle

    static constexpr int kHopSize    = 512;        // SAF convolver block size
    static constexpr int kNCHin      = 4;           // FOA channels (W Y Z X)
    static constexpr int kNCHout     = 2;           // binaural ears
    static constexpr int kRingFrames = kHopSize * 8; // output ring buffer depth

    // Staging buffer: accumulates rotated B-format in planar layout [nCH][kHopSize]
    std::vector<float> m_inStage;
    std::vector<float> m_outStage;  // planar output from convolver [nCH][kHopSize]
    int m_stageCount = 0;           // frames currently in staging buffer

    // Output ring buffer: interleaved stereo, kRingFrames deep
    std::vector<float> m_outRing;
    int m_ringWrite = 0;
    int m_ringRead  = 0;
    int m_ringCount = 0;
};
