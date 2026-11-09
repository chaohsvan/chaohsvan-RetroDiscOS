#include <string>
#include <vector>

#include "audio/mock_audio.h"
#include "cd/mock_cd.h"
#include "core/event_bus.h"
#include "core/player_core.h"
#include "test_util.h"

using namespace retrodisc;  // NOLINT: test readability

namespace {

struct Fixture {
    MockCDConfig cdConfig;
    MockCDBackend cd;
    MockAudioBackend audio;
    EventBus bus;
    PlayerCore core;
    std::vector<PlayerState> states;
    std::vector<int> tracks;
    std::vector<std::string> errors;

    explicit Fixture(MockCDConfig config = {})
        : cdConfig(config), cd(config), core(cd, audio, bus) {
        audio.setRealtime(false);
        cd.open();
        audio.open("null");
        core.setChunkFrames(kFramesPerSector * kSectorsPerSecond / 2);  // 0.5 s
        bus.subscribe([this](const Event& event) {
            if (event.type == EventType::StateChanged) {
                states.push_back(event.state);
            } else if (event.type == EventType::TrackChanged) {
                tracks.push_back(event.track);
            } else if (event.type == EventType::Error) {
                errors.push_back(event.message);
            }
        });
    }

    // Runs ticks until the predicate holds or the budget is exhausted.
    bool tickUntil(int maxTicks, bool (*predicate)(const PlayerCore&)) {
        for (int i = 0; i < maxTicks; ++i) {
            if (predicate(core)) return true;
            if (core.tick() <= 0 && core.state() != PlayerState::Playing) return predicate(core);
        }
        return predicate(core);
    }
};

MockCDConfig shortDisc() {
    MockCDConfig config;
    config.trackCount = 3;
    config.trackSeconds = 2;
    config.lastTrackSeconds = 2;
    return config;
}

bool isStopped(const PlayerCore& core) {
    return core.state() == PlayerState::Stopped;
}
bool onTrack3(const PlayerCore& core) {
    return core.trackNumber() == 3;
}

}  // namespace

RD_TEST(mock_flow_reaches_playing) {
    Fixture f(shortDisc());
    f.cd.insertDisc();

    RD_CHECK(f.core.insertDisc());
    RD_CHECK(f.core.state() == PlayerState::DiscReady);
    RD_CHECK_EQ(f.core.trackNumber(), 1);
    RD_CHECK_EQ(f.core.trackCount(), 3);
    RD_CHECK_EQ(f.core.currentTitle(), std::string("Mock Track 1"));

    RD_CHECK(f.core.play());
    RD_CHECK(f.core.state() == PlayerState::Playing);

    RD_CHECK(f.core.tick() > 0);
    RD_CHECK(f.core.elapsedMs() > 0);
}

RD_TEST(toc_failure_is_reported) {
    MockCDConfig config = shortDisc();
    config.failOnToc = true;
    Fixture f(config);
    f.cd.insertDisc();

    RD_CHECK(!f.core.insertDisc());
    RD_CHECK(f.core.state() == PlayerState::NoDisc);
    RD_CHECK(!f.errors.empty());
}

RD_TEST(insert_without_disc_fails) {
    Fixture f(shortDisc());
    RD_CHECK(!f.core.insertDisc());
    RD_CHECK(f.core.state() == PlayerState::NoDisc);
    RD_CHECK(!f.errors.empty());
}

RD_TEST(double_insert_is_rejected) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    RD_CHECK(f.core.insertDisc());
    RD_CHECK(!f.core.insertDisc());
}

RD_TEST(play_without_disc_fails) {
    Fixture f(shortDisc());
    RD_CHECK(!f.core.play());
    RD_CHECK(f.core.state() == PlayerState::NoDisc);
}

RD_TEST(sequential_playback_advances_tracks) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();

    RD_CHECK(f.tickUntil(100, onTrack3));
    RD_CHECK_EQ(f.core.trackNumber(), 3);
    RD_CHECK(f.core.state() == PlayerState::Playing);

    // Track change events were published: 1 on insert, then 2 and 3.
    RD_CHECK_EQ(static_cast<int>(f.tracks.size()), 3);
    RD_CHECK_EQ(f.tracks.at(0), 1);
    RD_CHECK_EQ(f.tracks.at(2), 3);
}

RD_TEST(end_of_disc_stops_playback) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();

    RD_CHECK(f.tickUntil(500, isStopped));
    RD_CHECK(f.core.state() == PlayerState::Stopped);
    RD_CHECK_EQ(f.core.tick(), 0);
}

RD_TEST(pause_keeps_position_and_resume_continues) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();

    RD_CHECK(f.core.tick() > 0);
    const int before = f.core.elapsedMs();
    RD_CHECK(before > 0);

    RD_CHECK(f.core.pause());
    RD_CHECK(f.core.state() == PlayerState::Paused);
    RD_CHECK_EQ(f.core.tick(), 0);
    RD_CHECK_EQ(f.core.elapsedMs(), before);

    RD_CHECK(f.core.play());
    RD_CHECK(f.core.state() == PlayerState::Playing);
    RD_CHECK(f.core.tick() > 0);
    RD_CHECK(f.core.elapsedMs() > before);
}

RD_TEST(next_and_prev_work_while_playing) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();

    RD_CHECK(f.core.next());
    RD_CHECK_EQ(f.core.trackNumber(), 2);
    RD_CHECK(f.core.state() == PlayerState::Playing);
    RD_CHECK_EQ(f.core.elapsedMs(), 0);

    RD_CHECK(f.core.prev());
    RD_CHECK_EQ(f.core.trackNumber(), 1);

    RD_CHECK(f.core.prev());  // restarts the first track
    RD_CHECK_EQ(f.core.trackNumber(), 1);
    RD_CHECK(f.core.tick() > 0);
}

RD_TEST(next_at_last_track_is_rejected) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();

    RD_CHECK(f.core.next());
    RD_CHECK(f.core.next());
    RD_CHECK_EQ(f.core.trackNumber(), 3);
    RD_CHECK(!f.core.next());
}

RD_TEST(stop_resets_position) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();
    f.core.tick();

    RD_CHECK(f.core.stop());
    RD_CHECK(f.core.state() == PlayerState::Stopped);
    RD_CHECK_EQ(f.core.elapsedMs(), 0);
}

RD_TEST(eject_ends_in_no_disc) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();
    f.core.tick();

    RD_CHECK(f.core.eject());
    RD_CHECK(f.core.state() == PlayerState::NoDisc);
    RD_CHECK(!f.core.hasDisc());
    RD_CHECK(!f.cd.hasDisc());  // the tray is open, nothing inside
    RD_CHECK_EQ(f.audio.state(), AudioState::Ready);
    RD_CHECK(!f.core.eject());  // already empty
}

RD_TEST(disc_removal_while_playing_is_safe) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();
    f.core.tick();

    f.cd.removeDisc();
    f.core.removeDisc();

    RD_CHECK(f.core.state() == PlayerState::NoDisc);
    RD_CHECK(f.audio.state() != AudioState::Running);
    RD_CHECK_EQ(f.core.tick(), 0);
}

RD_TEST(read_failure_stops_playback_with_an_error) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();

    f.cd.setFailOnRead(true);
    RD_CHECK_EQ(f.core.tick(), 0);
    RD_CHECK(f.core.state() == PlayerState::Stopped);
    RD_CHECK(!f.errors.empty());
}

RD_TEST(state_sequence_is_coherent) {
    Fixture f(shortDisc());
    f.cd.insertDisc();
    f.core.insertDisc();
    f.core.play();
    f.core.pause();
    f.core.play();
    f.core.stop();
    f.core.eject();

    const std::vector<PlayerState> expected = {
        PlayerState::DiscDetected, PlayerState::DiscReady, PlayerState::Playing,
        PlayerState::Paused,       PlayerState::Playing,   PlayerState::Stopped,
        PlayerState::Ejecting,     PlayerState::NoDisc,
    };
    RD_CHECK_EQ(f.states.size(), expected.size());
    for (std::size_t i = 0; i < expected.size() && i < f.states.size(); ++i) {
        RD_CHECK(f.states[i] == expected[i]);
    }
}

RD_TEST_MAIN()
