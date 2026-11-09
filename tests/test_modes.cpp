#include <string>
#include <vector>

#include "audio/mock_audio.h"
#include "cd/mock_cd.h"
#include "core/event_bus.h"
#include "core/play_mode.h"
#include "core/player_core.h"
#include "test_util.h"

using namespace retrodisc;  // NOLINT: test readability

namespace {

struct Fixture {
    MockCDBackend cd{[] {
        MockCDConfig config;
        config.trackCount = 3;
        config.trackSeconds = 2;
        config.lastTrackSeconds = 2;
        return config;
    }()};
    MockAudioBackend audio;
    EventBus bus;
    PlayerCore core{cd, audio, bus};

    Fixture() {
        audio.setRealtime(false);
        cd.open();
        audio.open("null");
        core.setChunkFrames(kFramesPerSector * kSectorsPerSecond / 2);
        cd.insertDisc();
        core.insertDisc();
    }
};

// Ticks until the elapsed time wraps back to zero (i.e. a track restarted).
bool tickUntilRestart(Fixture& f, int maxTicks) {
    int last = 0;
    for (int i = 0; i < maxTicks; ++i) {
        f.core.tick();
        const int now = f.core.elapsedMs();
        if (last > 0 && now == 0) return true;
        last = now;
    }
    return false;
}

}  // namespace

RD_TEST(mode_cycle_order) {
    RD_CHECK(nextPlayMode(PlayMode::Normal) == PlayMode::RepeatAll);
    RD_CHECK(nextPlayMode(PlayMode::RepeatAll) == PlayMode::RepeatOne);
    RD_CHECK(nextPlayMode(PlayMode::RepeatOne) == PlayMode::Random);
    RD_CHECK(nextPlayMode(PlayMode::Random) == PlayMode::Normal);
}

RD_TEST(mode_names) {
    RD_CHECK_EQ(std::string(toString(PlayMode::Normal)), std::string("NORMAL"));
    RD_CHECK_EQ(std::string(toString(PlayMode::RepeatAll)), std::string("REPEAT ALL"));
    RD_CHECK_EQ(std::string(toString(PlayMode::RepeatOne)), std::string("REPEAT ONE"));
    RD_CHECK_EQ(std::string(toString(PlayMode::Random)), std::string("RANDOM"));
}

RD_TEST(default_mode_is_normal) {
    Fixture f;
    RD_CHECK(f.core.playMode() == PlayMode::Normal);
}

RD_TEST(normal_mode_advances_then_stops_at_the_end) {
    Fixture f;
    f.core.play();
    RD_CHECK(f.core.next());  // track 2
    RD_CHECK(f.core.next());  // track 3
    RD_CHECK_EQ(f.core.trackNumber(), 3);

    for (int i = 0; i < 100 && f.core.state() == PlayerState::Playing; ++i) f.core.tick();
    RD_CHECK(f.core.state() == PlayerState::Stopped);
}

RD_TEST(repeat_one_restarts_the_same_track) {
    Fixture f;
    f.core.setPlayMode(PlayMode::RepeatOne);
    f.core.play();

    RD_CHECK(tickUntilRestart(f, 100));
    RD_CHECK_EQ(f.core.trackNumber(), 1);
    RD_CHECK(f.core.state() == PlayerState::Playing);
}

RD_TEST(repeat_all_wraps_from_last_track_to_first) {
    Fixture f;
    f.core.setPlayMode(PlayMode::RepeatAll);
    f.core.play();
    f.core.next();
    f.core.next();
    RD_CHECK_EQ(f.core.trackNumber(), 3);

    for (int i = 0; i < 100 && f.core.trackNumber() == 3; ++i) f.core.tick();
    RD_CHECK_EQ(f.core.trackNumber(), 1);
    RD_CHECK(f.core.state() == PlayerState::Playing);
}

RD_TEST(repeat_all_wraps_backwards_with_prev) {
    Fixture f;
    f.core.setPlayMode(PlayMode::RepeatAll);
    f.core.play();
    RD_CHECK_EQ(f.core.trackNumber(), 1);

    RD_CHECK(f.core.prev());
    RD_CHECK_EQ(f.core.trackNumber(), 3);
}

RD_TEST(random_mode_never_repeats_the_current_track) {
    Fixture f;
    f.core.setPlayMode(PlayMode::Random);
    f.core.seedRandom(1234);
    f.core.play();

    int previous = f.core.trackNumber();
    for (int i = 0; i < 20; ++i) {
        RD_CHECK(f.core.next());
        const int current = f.core.trackNumber();
        RD_CHECK(current >= 1 && current <= 3);
        RD_CHECK(current != previous);
        previous = current;
    }
}

RD_TEST(random_mode_advances_at_track_end) {
    Fixture f;
    f.core.setPlayMode(PlayMode::Random);
    f.core.seedRandom(99);
    f.core.play();

    const int before = f.core.trackNumber();
    bool changed = false;
    for (int i = 0; i < 100; ++i) {
        f.core.tick();
        if (f.core.trackNumber() != before) {
            changed = true;
            break;
        }
    }
    RD_CHECK(changed);
    RD_CHECK(f.core.state() == PlayerState::Playing);
}

RD_TEST(random_mode_with_single_track_stays_put) {
    MockCDConfig config;
    config.trackCount = 1;
    config.trackSeconds = 1;
    config.lastTrackSeconds = 1;
    MockCDBackend cd(config);
    MockAudioBackend audio;
    EventBus bus;
    PlayerCore core(cd, audio, bus);
    audio.setRealtime(false);
    cd.open();
    audio.open("null");
    core.setChunkFrames(kFramesPerSector * kSectorsPerSecond / 2);
    cd.insertDisc();
    core.insertDisc();
    core.setPlayMode(PlayMode::Random);
    core.play();

    for (int i = 0; i < 10; ++i) core.tick();
    RD_CHECK_EQ(core.trackNumber(), 1);
}

RD_TEST_MAIN()
