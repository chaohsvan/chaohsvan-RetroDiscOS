#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "audio/mock_audio.h"
#include "test_util.h"

using retrodisc::AudioState;
using retrodisc::kChannels;
using retrodisc::kSampleRate;
using retrodisc::MockAudioBackend;

namespace {

std::vector<char> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

uint32_t le32(const std::vector<char>& data, std::size_t offset) {
    if (offset + 4 > data.size()) return 0;
    return static_cast<uint32_t>(static_cast<unsigned char>(data[offset])) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[offset + 1])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[offset + 2])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(data[offset + 3])) << 24);
}

std::string uniquePath(const char* name) {
    return std::string("/tmp/retrodisc_test_") + name;
}

}  // namespace

RD_TEST(open_requires_a_device) {
    MockAudioBackend audio;
    RD_CHECK(!audio.open(""));
    RD_CHECK(audio.state() == AudioState::Closed);

    RD_CHECK(audio.open("null"));
    RD_CHECK(audio.state() == AudioState::Ready);
    RD_CHECK_EQ(audio.sampleRate(), kSampleRate);
    RD_CHECK_EQ(audio.channels(), kChannels);
}

RD_TEST(write_before_start_is_rejected) {
    MockAudioBackend audio;
    audio.setRealtime(false);
    audio.open("null");

    int16_t frame[kChannels] = {1, 2};
    RD_CHECK_EQ(audio.write(frame, 1), -1);
    RD_CHECK_EQ(audio.framesWritten(), 0LL);
}

RD_TEST(state_machine_start_pause_stop) {
    MockAudioBackend audio;
    audio.setRealtime(false);
    audio.open("null");

    RD_CHECK(audio.start());
    RD_CHECK(audio.state() == AudioState::Running);
    RD_CHECK(audio.pause());
    RD_CHECK(audio.state() == AudioState::Paused);
    RD_CHECK(audio.start());
    RD_CHECK(audio.state() == AudioState::Running);
    RD_CHECK(audio.stop());
    RD_CHECK(audio.state() == AudioState::Ready);
    RD_CHECK(audio.pause() == false);  // cannot pause when ready
}

RD_TEST(counts_frames_written) {
    MockAudioBackend audio;
    audio.setRealtime(false);
    audio.open("null");
    audio.start();

    int16_t frames[100 * kChannels] = {};
    RD_CHECK_EQ(audio.write(frames, 100), 100);
    RD_CHECK_EQ(audio.write(frames, 50), 50);
    RD_CHECK_EQ(audio.framesWritten(), 150LL);
}

RD_TEST(volume_is_clamped_to_unit_range) {
    MockAudioBackend audio;
    audio.setRealtime(false);
    audio.open("null");

    RD_CHECK(audio.setVolume(0.5f));
    RD_CHECK(audio.volume() > 0.49f && audio.volume() < 0.51f);

    RD_CHECK(!audio.setVolume(2.0f));
    RD_CHECK(audio.volume() == 1.0f);

    RD_CHECK(!audio.setVolume(-1.0f));
    RD_CHECK(audio.volume() == 0.0f);
}

RD_TEST(wav_file_contains_header_and_samples) {
    const std::string path = uniquePath("tone.wav");
    std::remove(path.c_str());

    {
        MockAudioBackend audio;
        audio.setRealtime(false);
        RD_CHECK(audio.open(path));
        RD_CHECK(audio.start());
        std::vector<int16_t> frames(256 * kChannels, 1000);
        RD_CHECK_EQ(audio.write(frames.data(), 256), 256);
        audio.close();
    }

    const std::vector<char> data = readFile(path);
    RD_CHECK(data.size() == 44u + 256u * kChannels * 2u);
    RD_CHECK_EQ(std::string(data.data(), 4), std::string("RIFF"));
    RD_CHECK_EQ(le32(data, 4), static_cast<uint32_t>(data.size() - 8));
    RD_CHECK_EQ(le32(data, 24), static_cast<uint32_t>(kSampleRate));
    RD_CHECK_EQ(le32(data, 40), static_cast<uint32_t>(256u * kChannels * 2u));
    std::remove(path.c_str());
}

RD_TEST(volume_gain_is_applied_to_samples) {
    const std::string path = uniquePath("half.wav");
    std::remove(path.c_str());

    {
        MockAudioBackend audio;
        audio.setRealtime(false);
        RD_CHECK(audio.open(path));
        RD_CHECK(audio.setVolume(0.5f));
        RD_CHECK(audio.start());
        std::vector<int16_t> frames(100 * kChannels, 1000);
        RD_CHECK_EQ(audio.write(frames.data(), 100), 100);
        audio.close();
    }

    const std::vector<char> data = readFile(path);
    RD_CHECK(data.size() > 44u + 4u);
    if (data.size() > 44u + 4u) {
        const auto sample = static_cast<int16_t>(static_cast<unsigned char>(data[44]) |
                                                 (static_cast<unsigned char>(data[45]) << 8));
        RD_CHECK(sample >= 490 && sample <= 510);
    }
    std::remove(path.c_str());
}

RD_TEST(realtime_pacing_tracks_the_clock) {
    MockAudioBackend audio;
    audio.setRealtime(true);
    audio.open("null");
    audio.start();

    // 0.5 s of audio, in 5 chunks of 0.1 s.
    std::vector<int16_t> chunk(static_cast<std::size_t>(kSampleRate / 10) * kChannels, 0);
    const auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < 5; ++i) {
        RD_CHECK_EQ(audio.write(chunk.data(), kSampleRate / 10), kSampleRate / 10);
    }
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - begin)
                               .count();

    RD_CHECK(elapsedMs >= 450);
    RD_CHECK(elapsedMs < 900);
}

RD_TEST(realtime_can_be_disabled_for_fast_tests) {
    MockAudioBackend audio;
    audio.setRealtime(false);
    audio.open("null");
    audio.start();

    std::vector<int16_t> chunk(static_cast<std::size_t>(kSampleRate) * kChannels, 0);
    const auto begin = std::chrono::steady_clock::now();
    RD_CHECK_EQ(audio.write(chunk.data(), kSampleRate), kSampleRate);
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - begin)
                               .count();
    RD_CHECK(elapsedMs < 100);
}

RD_TEST(close_returns_to_closed_state) {
    MockAudioBackend audio;
    audio.setRealtime(false);
    audio.open("null");
    audio.start();
    audio.close();

    RD_CHECK(audio.state() == AudioState::Closed);
    int16_t frame[kChannels] = {0, 0};
    RD_CHECK_EQ(audio.write(frame, 1), -1);
    RD_CHECK_EQ(audio.framesWritten(), 0LL);
}

RD_TEST_MAIN()
