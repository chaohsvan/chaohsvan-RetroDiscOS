#include <vector>

#include "cd/mock_cd.h"
#include "test_util.h"

using retrodisc::DiscInfo;
using retrodisc::formatTime;
using retrodisc::formatTrackTitle;
using retrodisc::MockCDBackend;
using retrodisc::MockCDConfig;

RD_TEST(mock_drive_requires_open_and_disc) {
    MockCDBackend cd;
    RD_CHECK(!cd.detectDisc());
    RD_CHECK(!cd.readTOC().valid);

    RD_CHECK(cd.open());
    RD_CHECK(cd.isOpen());
    RD_CHECK(!cd.detectDisc());  // open but empty

    cd.insertDisc();
    RD_CHECK(cd.detectDisc());
    RD_CHECK(cd.readTOC().valid);
}

RD_TEST(toc_has_expected_shape) {
    MockCDBackend cd;
    cd.open();
    cd.insertDisc();

    const DiscInfo info = cd.readTOC();
    RD_CHECK(info.valid);
    RD_CHECK_EQ(info.trackCount, 12);
    RD_CHECK_EQ(static_cast<int>(info.tracks.size()), 12);

    int expectedTotal = 0;
    for (int i = 1; i <= 12; ++i) {
        const auto* track = info.track(i);
        RD_CHECK(track != nullptr);
        if (track == nullptr) continue;
        RD_CHECK_EQ(track->number, i);
        RD_CHECK_EQ(track->startLba, expectedTotal);
        expectedTotal += track->lengthSectors;
    }
    RD_CHECK_EQ(info.totalSectors, expectedTotal);

    // Last track is longer than the others (5:10 vs 3:45).
    const auto* first = info.track(1);
    const auto* last = info.track(12);
    RD_CHECK(first != nullptr && last != nullptr);
    if (first != nullptr && last != nullptr) {
        RD_CHECK(last->lengthSectors > first->lengthSectors);
    }
}

RD_TEST(cd_text_fallback_naming) {
    MockCDBackend cd;
    cd.open();
    cd.insertDisc();
    const DiscInfo info = cd.readTOC();

    const auto* withText = info.track(1);
    const auto* without = info.track(2);
    RD_CHECK(withText != nullptr && without != nullptr);
    if (withText == nullptr || without == nullptr) return;

    RD_CHECK(!withText->title.empty());
    RD_CHECK(withText->title == "Mock Track 1");
    RD_CHECK(without->title.empty());
    RD_CHECK_EQ(formatTrackTitle(*without), std::string("TRACK 02"));
}

RD_TEST(read_frames_returns_audio) {
    MockCDBackend cd;
    cd.open();
    cd.insertDisc();

    std::vector<int16_t> pcm;
    const int frames = cd.readFrames(1, 0, 1000, pcm);
    RD_CHECK_EQ(frames, 1000);
    RD_CHECK_EQ(pcm.size(), static_cast<std::size_t>(1000 * retrodisc::kChannels));

    bool anyNonZero = false;
    for (int16_t sample : pcm) {
        if (sample != 0) {
            anyNonZero = true;
            break;
        }
    }
    RD_CHECK(anyNonZero);
}

RD_TEST(read_frames_clamps_at_track_end) {
    MockCDBackend cd;
    cd.open();
    cd.insertDisc();

    const DiscInfo info = cd.readTOC();
    const auto* track = info.track(1);
    RD_CHECK(track != nullptr);
    if (track == nullptr) return;

    std::vector<int16_t> pcm;
    const int frames =
        cd.readFrames(1, 0, track->lengthSectors * retrodisc::kFramesPerSector + 100, pcm);
    RD_CHECK_EQ(frames, track->lengthSectors * retrodisc::kFramesPerSector);
}

RD_TEST(read_frames_rejects_bad_requests) {
    MockCDBackend cd;
    std::vector<int16_t> pcm;

    // Not open.
    RD_CHECK_EQ(cd.readFrames(1, 0, 100, pcm), -1);

    cd.open();
    // No disc.
    RD_CHECK_EQ(cd.readFrames(1, 0, 100, pcm), -1);

    cd.insertDisc();
    // Unknown track.
    RD_CHECK_EQ(cd.readFrames(99, 0, 100, pcm), -1);
    // Offset past the end.
    RD_CHECK_EQ(cd.readFrames(1, 999999, 100, pcm), -1);
    // Zero-length request.
    RD_CHECK_EQ(cd.readFrames(1, 0, 0, pcm), -1);
}

RD_TEST(read_failure_injection) {
    MockCDBackend cd;
    cd.open();
    cd.insertDisc();
    cd.setFailOnRead(true);

    std::vector<int16_t> pcm;
    RD_CHECK_EQ(cd.readFrames(1, 0, 100, pcm), -1);
    RD_CHECK(pcm.empty());
}

RD_TEST(toc_failure_injection) {
    MockCDConfig config;
    config.failOnToc = true;
    MockCDBackend cd(config);
    cd.open();
    cd.insertDisc();
    RD_CHECK(!cd.readTOC().valid);
}

RD_TEST(eject_removes_the_disc_but_keeps_the_drive_open) {
    MockCDBackend cd;
    cd.open();
    cd.insertDisc();
    RD_CHECK(cd.detectDisc());

    RD_CHECK(cd.eject());
    RD_CHECK(!cd.detectDisc());
    RD_CHECK(cd.isOpen());  // the tray is open, the backend is still usable
}

RD_TEST(disc_change_is_observable) {
    MockCDBackend cd;
    cd.open();
    cd.insertDisc();
    RD_CHECK(cd.readTOC().valid);

    cd.removeDisc();
    RD_CHECK(!cd.detectDisc());
    RD_CHECK(!cd.readTOC().valid);

    cd.insertDisc();
    RD_CHECK(cd.readTOC().valid);
}

RD_TEST(open_can_fail) {
    MockCDConfig config;
    config.failOnOpen = true;
    MockCDBackend cd(config);
    RD_CHECK(!cd.open());
    RD_CHECK(!cd.isOpen());
    RD_CHECK(!cd.detectDisc());
}

RD_TEST(time_formatting) {
    RD_CHECK_EQ(formatTime(0), std::string("00:00"));
    RD_CHECK_EQ(formatTime(225000), std::string("03:45"));
    RD_CHECK_EQ(formatTime(310000), std::string("05:10"));
    RD_CHECK_EQ(formatTime(-5), std::string("00:00"));
}

RD_TEST_MAIN()
