#include <cstdio>
#include <string>

#include "audio/mock_audio.h"
#include "cd/mock_cd.h"
#include "core/event_bus.h"
#include "core/player_core.h"
#include "core/resume.h"
#include "core/state_store.h"
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

const char* kStatePath = "/tmp/retrodisc_resume_test.conf";

}  // namespace

RD_TEST(resume_round_trip) {
    std::remove(kStatePath);
    {
        StateStore store(kStatePath);
        store.load();
        ResumePoint point;
        point.discId = "MOCK-DISC-0001";
        point.track = 3;
        point.positionMs = 45100;
        saveResume(store, point);
    }
    StateStore reloaded(kStatePath);
    RD_CHECK(reloaded.load());
    const ResumePoint loaded = loadResume(reloaded);
    RD_CHECK(loaded.valid());
    RD_CHECK_EQ(loaded.discId, std::string("MOCK-DISC-0001"));
    RD_CHECK_EQ(loaded.track, 3);
    RD_CHECK_EQ(loaded.positionMs, 45100);
    std::remove(kStatePath);
}

RD_TEST(empty_store_has_no_resume_point) {
    StateStore store("/tmp/retrodisc_resume_missing.conf");
    store.load();
    const ResumePoint point = loadResume(store);
    RD_CHECK(!point.valid());
    RD_CHECK_EQ(point.track, 1);
    RD_CHECK_EQ(point.positionMs, 0);
}

RD_TEST(clear_removes_resume_data) {
    std::remove(kStatePath);
    StateStore store(kStatePath);
    store.load();
    ResumePoint point;
    point.discId = "MOCK-DISC-0001";
    point.track = 2;
    saveResume(store, point);
    RD_CHECK(!loadResume(store).discId.empty());

    clearResume(store);
    StateStore reloaded(kStatePath);
    RD_CHECK(reloaded.load());
    RD_CHECK(!loadResume(reloaded).valid());
    std::remove(kStatePath);
}

RD_TEST(corrupt_numbers_fall_back_to_defaults) {
    StateStore store("/tmp/retrodisc_resume_corrupt.conf");
    store.load();
    store.set("resume.disc", "X");
    store.set("resume.track", "banana");
    store.set("resume.position", "-42");
    const ResumePoint point = loadResume(store);
    RD_CHECK_EQ(point.track, 1);
    RD_CHECK_EQ(point.positionMs, 0);
}

RD_TEST(apply_resume_restores_track_and_position) {
    Fixture f;
    ResumePoint point;
    point.discId = f.core.disc().discId;
    point.track = 2;
    point.positionMs = 1000;

    RD_CHECK(f.core.applyResume(point));
    RD_CHECK_EQ(f.core.trackNumber(), 2);
    RD_CHECK(f.core.elapsedMs() >= 900);
    RD_CHECK(f.core.elapsedMs() <= 1100);
}

RD_TEST(apply_resume_ignores_other_discs) {
    Fixture f;
    ResumePoint point;
    point.discId = "SOME-OTHER-DISC";
    point.track = 2;
    RD_CHECK(!f.core.applyResume(point));
    RD_CHECK_EQ(f.core.trackNumber(), 1);
}

RD_TEST(apply_resume_clamps_track_number) {
    Fixture f;
    ResumePoint point;
    point.discId = f.core.disc().discId;
    point.track = 99;
    RD_CHECK(f.core.applyResume(point));
    RD_CHECK_EQ(f.core.trackNumber(), 3);

    point.track = -5;
    RD_CHECK(f.core.applyResume(point));
    RD_CHECK_EQ(f.core.trackNumber(), 1);
}

RD_TEST(apply_resume_past_track_end_restarts_track) {
    Fixture f;
    ResumePoint point;
    point.discId = f.core.disc().discId;
    point.track = 1;
    point.positionMs = 999999;
    RD_CHECK(f.core.applyResume(point));
    RD_CHECK_EQ(f.core.elapsedMs(), 0);
}

RD_TEST(resume_point_reflects_current_position) {
    Fixture f;
    f.core.play();
    f.core.tick();
    const ResumePoint point = f.core.resumePoint();
    RD_CHECK_EQ(point.discId, f.core.disc().discId);
    RD_CHECK_EQ(point.track, 1);
    RD_CHECK(point.positionMs > 0);
}

RD_TEST(apply_resume_rejected_while_playing) {
    Fixture f;
    f.core.play();
    ResumePoint point;
    point.discId = f.core.disc().discId;
    point.track = 2;
    RD_CHECK(!f.core.applyResume(point));
}

RD_TEST(restored_position_survives_play) {
    Fixture f;
    ResumePoint point;
    point.discId = f.core.disc().discId;
    point.track = 2;
    point.positionMs = 1200;
    RD_CHECK(f.core.applyResume(point));

    RD_CHECK(f.core.play());
    RD_CHECK(f.core.elapsedMs() >= 1100);

    f.core.tick();
    RD_CHECK(f.core.elapsedMs() >= 1500);
}

RD_TEST_MAIN()
