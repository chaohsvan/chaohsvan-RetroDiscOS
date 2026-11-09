#include <algorithm>
#include <cmath>
#include <vector>

#include "core/spectrum.h"
#include "test_util.h"

using retrodisc::SpectrumAnalyzer;

namespace {

std::vector<int16_t> makeTone(double frequency, int frames, double amplitude = 0.5) {
    std::vector<int16_t> samples(static_cast<std::size_t>(frames) * 2);
    for (int i = 0; i < frames; ++i) {
        const double t = static_cast<double>(i) / 44100.0;
        const auto value =
            static_cast<int16_t>(32767.0 * amplitude * std::sin(2.0 * M_PI * frequency * t));
        samples[static_cast<std::size_t>(i) * 2] = value;
        samples[static_cast<std::size_t>(i) * 2 + 1] = value;
    }
    return samples;
}

int loudestBand(const SpectrumAnalyzer& spectrum) {
    int best = 0;
    for (int i = 1; i < SpectrumAnalyzer::kBands; ++i) {
        if (spectrum.levels()[static_cast<std::size_t>(i)] >
            spectrum.levels()[static_cast<std::size_t>(best)]) {
            best = i;
        }
    }
    return best;
}

}  // namespace

RD_TEST(bands_are_log_spaced_and_ordered) {
    RD_CHECK(SpectrumAnalyzer::bandFrequency(0) < 100.0f);
    RD_CHECK(SpectrumAnalyzer::bandFrequency(SpectrumAnalyzer::kBands - 1) > 10000.0f);
    for (int i = 1; i < SpectrumAnalyzer::kBands; ++i) {
        RD_CHECK(SpectrumAnalyzer::bandFrequency(i) > SpectrumAnalyzer::bandFrequency(i - 1));
    }
}

RD_TEST(initial_levels_are_zero) {
    SpectrumAnalyzer spectrum;
    for (float level : spectrum.levels()) RD_CHECK_EQ(level, 0.0f);
}

RD_TEST(silence_keeps_levels_low) {
    SpectrumAnalyzer spectrum;
    std::vector<int16_t> silence(2048 * 2, 0);
    spectrum.push(silence.data(), 2048);

    for (float level : spectrum.levels()) RD_CHECK(level < 0.02f);
}

RD_TEST(one_khz_tone_lights_the_matching_band) {
    SpectrumAnalyzer spectrum;
    const auto tone = makeTone(1000.0, SpectrumAnalyzer::kWindow * 2);
    spectrum.push(tone.data(), static_cast<int>(tone.size() / 2));

    const int band = loudestBand(spectrum);
    const float frequency = SpectrumAnalyzer::bandFrequency(band);
    RD_CHECK(frequency > 400.0f);
    RD_CHECK(frequency < 2500.0f);
    RD_CHECK(spectrum.levels()[static_cast<std::size_t>(band)] > 0.2f);
}

RD_TEST(levels_stay_within_range) {
    SpectrumAnalyzer spectrum;
    const auto loud = makeTone(440.0, 4096, 1.0);
    spectrum.push(loud.data(), static_cast<int>(loud.size() / 2));
    for (float level : spectrum.levels()) {
        RD_CHECK(level >= 0.0f);
        RD_CHECK(level <= 1.0f);
    }
}

RD_TEST(levels_decay_after_silence) {
    SpectrumAnalyzer spectrum;
    const auto tone = makeTone(1000.0, SpectrumAnalyzer::kWindow * 2);
    spectrum.push(tone.data(), static_cast<int>(tone.size() / 2));
    const int band = loudestBand(spectrum);
    const float before = spectrum.levels()[static_cast<std::size_t>(band)];
    RD_CHECK(before > 0.1f);

    std::vector<int16_t> silence(4096 * 2, 0);
    for (int i = 0; i < 5; ++i) spectrum.push(silence.data(), 4096);

    const float after = spectrum.levels()[static_cast<std::size_t>(band)];
    RD_CHECK(after < before);
    RD_CHECK(after >= 0.0f);
}

RD_TEST(reset_clears_everything) {
    SpectrumAnalyzer spectrum;
    const auto tone = makeTone(1000.0, 4096);
    spectrum.push(tone.data(), 4096);
    spectrum.reset();
    for (float level : spectrum.levels()) RD_CHECK_EQ(level, 0.0f);
}

RD_TEST(null_input_is_ignored) {
    SpectrumAnalyzer spectrum;
    spectrum.push(nullptr, 100);
    for (float level : spectrum.levels()) RD_CHECK_EQ(level, 0.0f);
}

RD_TEST_MAIN()
