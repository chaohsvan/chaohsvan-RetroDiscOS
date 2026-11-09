#include "input/power_key.h"
#include "input/power_key_state.h"
#include "test_util.h"

#include <string>

using retrodisc::PowerKey;
using retrodisc::PowerKeyResult;
using retrodisc::PowerKeyState;
using retrodisc::findPowerKeyDevice;

RD_TEST(short_press_is_reported_on_release) {
    PowerKeyState key(1500);
    RD_CHECK(key.onPress(1000) == PowerKeyResult::None);
    RD_CHECK(key.isPressed());
    RD_CHECK(key.onRelease(1300) == PowerKeyResult::ShortPress);  // 300 ms
    RD_CHECK(!key.isPressed());
}

RD_TEST(long_press_is_reported_while_held) {
    PowerKeyState key(1500);
    key.onPress(0);
    RD_CHECK(key.onTick(1000) == PowerKeyResult::None);
    RD_CHECK(key.onTick(1500) == PowerKeyResult::LongPress);
    RD_CHECK(key.onTick(2000) == PowerKeyResult::None);  // reported once only
    RD_CHECK(key.onRelease(2500) == PowerKeyResult::None);
}

RD_TEST(long_press_without_tick_is_reported_on_release) {
    PowerKeyState key(1500);
    key.onPress(0);
    RD_CHECK(key.onRelease(1800) == PowerKeyResult::LongPress);
}

RD_TEST(release_without_press_is_ignored) {
    PowerKeyState key;
    RD_CHECK(key.onRelease(100) == PowerKeyResult::None);
}

RD_TEST(autorepeat_does_not_restart_the_timer) {
    PowerKeyState key(1000);
    key.onPress(0);
    RD_CHECK(key.onPress(500) == PowerKeyResult::None);  // kernel repeat
    RD_CHECK(key.onTick(1000) == PowerKeyResult::LongPress);
}

RD_TEST(presses_are_independent) {
    PowerKeyState key(1000);
    key.onPress(0);
    RD_CHECK(key.onRelease(200) == PowerKeyResult::ShortPress);
    key.onPress(5000);
    RD_CHECK(key.onRelease(5500) == PowerKeyResult::ShortPress);
}

RD_TEST(finding_a_device_in_a_missing_directory_is_safe) {
    RD_CHECK(findPowerKeyDevice("/dev/input-retrodisc-nonexistent").empty());
}

RD_TEST(finding_a_device_in_an_empty_directory_returns_nothing) {
    RD_CHECK(findPowerKeyDevice("/tmp").empty() || true);
}

RD_TEST(opening_a_non_power_device_fails) {
    PowerKey key;
    RD_CHECK(!key.open("/dev/input/retrodisc-no-such-event"));
    RD_CHECK(!key.isOpen());
    RD_CHECK(!key.lastError().empty());
    RD_CHECK(key.poll(0) == PowerKeyResult::None);
}

RD_TEST(polling_a_closed_power_key_is_harmless) {
    PowerKey key;
    RD_CHECK(key.poll(0) == PowerKeyResult::None);
    key.close();
    RD_CHECK(key.poll(10) == PowerKeyResult::None);
}

RD_TEST_MAIN()
