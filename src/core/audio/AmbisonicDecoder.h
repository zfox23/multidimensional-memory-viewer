#pragma once
#include <vector>

enum class AmbisonicFormat {
    AmbiX,  // Channel order: W, Y, Z, X  (Zoom H2n Native/ACN/SN3D)
    FuMa    // Channel order: W, X, Y, Z  (Legacy)
};

// First-Order Ambisonics → stereo binaural decoder using the SAF ambi_bin
// example plugin with the built-in KEMAR HRTF dataset.
class AmbisonicDecoder {
public:
    explicit AmbisonicDecoder(AmbisonicFormat format = AmbisonicFormat::AmbiX);
    ~AmbisonicDecoder();

    void setFormat(AmbisonicFormat format);
    // Orientation in radians; converted to degrees internally.
    void setOrientation(float yaw, float pitch, float roll = 0.0f);

    // Call before each new playback to clear stale staging state and trigger fade-in.
    void reset();

    // Process one block of interleaved 4-channel input → interleaved 2-channel output.
    void process(const float* input4ch, float* output2ch, int frameCount);

private:
    void applyFormat();

    void* m_hAmbi = nullptr;
    AmbisonicFormat m_format;

    static constexpr int kNCHin        = 4;
    static constexpr int kNCHout       = 2;
    static constexpr int kFadeInFrames = 2400;  // 50 ms @ 48 kHz

    int m_frameSize = 128;

    // Planar staging buffers pointed to by m_inPtrs / m_outPtrs
    std::vector<float> m_inBuf;
    std::vector<float> m_outBuf;
    int m_inCount = 0;

    float* m_inPtrs[kNCHin]   = {};
    float* m_outPtrs[kNCHout] = {};

    // Spill buffer: holds at most one frameSize worth of output that didn't
    // fit in the caller's out2ch when m_inCount carried over from the previous call.
    std::vector<float> m_spillBuf;  // interleaved stereo, capacity = frameSize * 2
    int m_spillCount = 0;           // frames currently held

    int m_fadeInRemaining = 0;  // output samples still needing fade-in ramp
};
