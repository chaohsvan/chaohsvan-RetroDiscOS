#pragma once

#include <array>
#include <cstdint>

namespace retrodisc {

// Cheap spectrum for the F9 display. One Goertzel filter per band over the most
// recent window; no FFT library, no new dependency (AGENTS.md 6).
class SpectrumAnalyzer {
   public:
    static constexpr int kBands = 16;
    static constexpr int kWindow = 1024;

    void reset();
    void push(const int16_t* frames, int frameCount);  // interleaved stereo

    const std::array<float, kBands>& levels() const {
        return levels_;
    }

    // Centre frequency of a band, log spaced from 60 Hz to 12 kHz.
    static float bandFrequency(int index);

   private:
    void recompute();

    std::array<float, kWindow> window_{};
    int filled_ = 0;
    int cursor_ = 0;
    std::array<float, kBands> levels_{};
};

}  // namespace retrodisc
