#include "cd/real_cd.h"
#include "test_util.h"

#include <string>
#include <vector>

using retrodisc::RealCDBackend;

RD_TEST(missing_device_fails_gracefully) {
    RealCDBackend cd("/dev/retrodisc-no-such-drive");
    RD_CHECK(!cd.open());
    RD_CHECK(!cd.lastError().empty());
    RD_CHECK(!cd.detectDisc());
    RD_CHECK(!cd.readTOC().valid);
}

RD_TEST(non_cdrom_file_is_rejected) {
    RealCDBackend cd("/dev/null");
    RD_CHECK(!cd.open());
    RD_CHECK(!cd.lastError().empty());
}

RD_TEST(reading_before_open_is_safe) {
    RealCDBackend cd;
    std::vector<int16_t> pcm;
    RD_CHECK(!cd.detectDisc());
    RD_CHECK(!cd.readTOC().valid);
    RD_CHECK_EQ(cd.readFrames(1, 0, 100, pcm), -1);
    RD_CHECK(pcm.empty());
}

RD_TEST(eject_on_missing_device_fails) {
    RealCDBackend cd("/dev/retrodisc-no-such-drive");
    RD_CHECK(!cd.eject());
    RD_CHECK(!cd.lastError().empty());
}

RD_TEST(close_is_idempotent) {
    RealCDBackend cd("/dev/retrodisc-no-such-drive");
    cd.close();
    cd.close();
    RD_CHECK(!cd.detectDisc());
}

RD_TEST(device_path_is_reported) {
    RealCDBackend cd("/dev/sr9");
    RD_CHECK_EQ(cd.device(), std::string("/dev/sr9"));
}

RD_TEST(cd_text_is_kept_only_when_printable_ascii) {
    RD_CHECK_EQ(retrodisc::sanitizeCdText("Waiting For You"),
                std::string("Waiting For You"));
    RD_CHECK_EQ(retrodisc::sanitizeCdText("TRACK 01"), std::string("TRACK 01"));
    RD_CHECK_EQ(retrodisc::sanitizeCdText(""), std::string(""));

    // GBK bytes from a Chinese disc: must not reach the renderer.
    RD_CHECK(retrodisc::sanitizeCdText("K\xd4\xc2\xd6\xae\xcd\xf5").empty());
    RD_CHECK(retrodisc::sanitizeCdText("abc\n").empty());  // control byte
}

RD_TEST_MAIN()
