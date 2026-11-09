#include "audio/alsa_audio.h"
#include "test_util.h"

#include <string>
#include <vector>

using retrodisc::ALSAAudioBackend;
using retrodisc::AudioState;
using retrodisc::kChannels;

namespace {

// The ALSA "null" plugin needs no hardware, but keep the suite honest on
// machines where alsa-lib is present without it.
bool nullDeviceUsable() {
    ALSAAudioBackend probe;
    return probe.open("null");
}

}  // namespace

RD_TEST(empty_device_name_is_rejected) {
    ALSAAudioBackend audio;
    RD_CHECK(!audio.open(""));
    RD_CHECK(audio.state() == AudioState::Closed);
    RD_CHECK(!audio.lastError().empty());
}

RD_TEST(unknown_device_is_reported) {
    ALSAAudioBackend audio;
    RD_CHECK(!audio.open("retrodisc-no-such-pcm-device"));
    RD_CHECK(audio.state() == AudioState::Closed);
    RD_CHECK(audio.lastError().find("snd_pcm_open") != std::string::npos);
}

RD_TEST(null_device_opens_and_reports_format) {
    if (!nullDeviceUsable()) {
        std::printf("  (skipped: ALSA null plugin unavailable)\n");
        return;
    }
    ALSAAudioBackend audio;
    RD_CHECK(audio.open("null"));
    RD_CHECK(audio.state() == AudioState::Ready);
    RD_CHECK_EQ(audio.sampleRate(), 44100);
    RD_CHECK_EQ(audio.channels(), 2);
    RD_CHECK_EQ(audio.device(), std::string("null"));
}

RD_TEST(write_requires_running_state) {
    if (!nullDeviceUsable()) return;
    ALSAAudioBackend audio;
    RD_CHECK(audio.open("null"));

    std::vector<int16_t> frames(256 * kChannels, 0);
    RD_CHECK_EQ(audio.write(frames.data(), 256), -1);

    RD_CHECK(audio.start());
    RD_CHECK_EQ(audio.state(), AudioState::Running);
    RD_CHECK_EQ(audio.write(frames.data(), 256), 256);
    RD_CHECK_EQ(audio.framesWritten(), 256LL);
}

RD_TEST(pause_blocks_writes_and_resumes) {
    if (!nullDeviceUsable()) return;
    ALSAAudioBackend audio;
    RD_CHECK(audio.open("null"));
    RD_CHECK(audio.start());

    std::vector<int16_t> frames(128 * kChannels, 0);
    RD_CHECK_EQ(audio.write(frames.data(), 128), 128);

    RD_CHECK(audio.pause());
    RD_CHECK_EQ(audio.state(), AudioState::Paused);
    RD_CHECK_EQ(audio.write(frames.data(), 128), -1);

    RD_CHECK(audio.start());
    RD_CHECK_EQ(audio.state(), AudioState::Running);
    RD_CHECK_EQ(audio.write(frames.data(), 128), 128);
    RD_CHECK_EQ(audio.framesWritten(), 256LL);
}

RD_TEST(stop_resets_position) {
    if (!nullDeviceUsable()) return;
    ALSAAudioBackend audio;
    RD_CHECK(audio.open("null"));
    RD_CHECK(audio.start());

    std::vector<int16_t> frames(64 * kChannels, 0);
    RD_CHECK_EQ(audio.write(frames.data(), 64), 64);

    RD_CHECK(audio.stop());
    RD_CHECK_EQ(audio.state(), AudioState::Ready);
    RD_CHECK_EQ(audio.framesWritten(), 0LL);
}

RD_TEST(volume_is_software_gain_and_clamped) {
    if (!nullDeviceUsable()) return;
    ALSAAudioBackend audio;
    RD_CHECK(audio.open("null"));
    RD_CHECK(audio.start());

    RD_CHECK(audio.setVolume(0.25f));
    RD_CHECK(audio.volume() > 0.24f && audio.volume() < 0.26f);

    // Values outside [0,1] are clamped and reported as invalid.
    RD_CHECK(!audio.setVolume(3.0f));
    RD_CHECK_EQ(audio.volume(), 1.0f);
    RD_CHECK(!audio.setVolume(-0.5f));
    RD_CHECK_EQ(audio.volume(), 0.0f);

    // Gain path must still write the full block.
    std::vector<int16_t> frames(100 * kChannels, 1000);
    RD_CHECK(audio.setVolume(0.5f));
    RD_CHECK_EQ(audio.write(frames.data(), 100), 100);
}

RD_TEST(close_invalidates_the_backend) {
    if (!nullDeviceUsable()) return;
    ALSAAudioBackend audio;
    RD_CHECK(audio.open("null"));
    RD_CHECK(audio.start());
    audio.close();

    RD_CHECK_EQ(audio.state(), AudioState::Closed);
    std::vector<int16_t> frames(kChannels, 0);
    RD_CHECK_EQ(audio.write(frames.data(), 1), -1);
    RD_CHECK(!audio.start());
}

RD_TEST(reopen_after_close_works) {
    if (!nullDeviceUsable()) return;
    ALSAAudioBackend audio;
    RD_CHECK(audio.open("null"));
    audio.close();
    RD_CHECK(audio.open("null"));
    RD_CHECK(audio.state() == AudioState::Ready);
    RD_CHECK(audio.start());
}

RD_TEST(zero_length_write_is_rejected) {
    if (!nullDeviceUsable()) return;
    ALSAAudioBackend audio;
    RD_CHECK(audio.open("null"));
    RD_CHECK(audio.start());
    std::vector<int16_t> frames(kChannels, 0);
    RD_CHECK_EQ(audio.write(frames.data(), 0), -1);
    RD_CHECK_EQ(audio.write(nullptr, 10), -1);
}

RD_TEST_MAIN()
