#include "core/spectrum.h"

#include <algorithm>
#include <cmath>

namespace retrodisc {
namespace {

constexpr double kMinHz = 60.0;
constexpr double kMaxHz = 12000.0;
constexpr float kDecay = 0.75f;
constexpr float kGain = 6.0f;

}  // namespace

float SpectrumAnalyzer::bandFrequency(int index) {
    if (index < 0) index = 0;
    if (index >= kBands) index = kBands - 1;
    const double ratio = kMaxHz / kMinHz;
    return static_cast<float>(kMinHz * std::pow(ratio, static_cast<double>(index) / (kBands - 1)));
}

void SpectrumAnalyzer::reset() {
    window_.fill(0.0f);
    levels_.fill(0.0f);
    filled_ = 0;
    cursor_ = 0;
}

void SpectrumAnalyzer::push(const int16_t* frames, int frameCount) {
    if (frames == nullptr || frameCount <= 0) return;

    for (int i = 0; i < frameCount; ++i) {
        const float left = static_cast<float>(frames[i * 2]) / 32768.0f;
        const float right = static_cast<float>(frames[i * 2 + 1]) / 32768.0f;
        window_[static_cast<std::size_t>(cursor_)] = 0.5f * (left + right);
        cursor_ = (cursor_ + 1) % kWindow;
        if (filled_ < kWindow) ++filled_;
    }
    recompute();
}

void SpectrumAnalyzer::recompute() {
    if (filled_ < kWindow / 8) return;  // not enough data yet

    // Chronological copy of the ring buffer.
    std::array<double, kWindow> samples{};
    for (int i = 0; i < filled_; ++i) {
        const int index = (cursor_ - filled_ + i + kWindow) % kWindow;
        samples[static_cast<std::size_t>(i)] =
            static_cast<double>(window_[static_cast<std::size_t>(index)]);
    }

    const double n = static_cast<double>(filled_);
    for (int band = 0; band < kBands; ++band) {
        const double omega = 2.0 * M_PI * bandFrequency(band) / 44100.0;
        const double coeff = 2.0 * std::cos(omega);
        double s0 = 0.0;
        double s1 = 0.0;
        double s2 = 0.0;
        for (int i = 0; i < filled_; ++i) {
            s0 = samples[static_cast<std::size_t>(i)] + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        const double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
        const double magnitude = std::sqrt(std::max(0.0, power)) / n;
        const auto instant = static_cast<float>(magnitude * kGain);
        const float decayed = levels_[static_cast<std::size_t>(band)] * kDecay;
        levels_[static_cast<std::size_t>(band)] =
            std::clamp(std::max(instant, decayed), 0.0f, 1.0f);
    }
}

}  // namespace retrodisc
