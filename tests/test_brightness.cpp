#include "display/backlight.h"
#include "display/brightness.h"
#include "display/display_power.h"
#include "test_util.h"

using retrodisc::BrightnessController;
using retrodisc::DisplayPower;
using retrodisc::MockBacklight;
using retrodisc::NullBacklight;
using retrodisc::SysfsBacklight;
using retrodisc::Theme;

RD_TEST(levels_are_the_five_documented_steps) {
    RD_CHECK_EQ(BrightnessController::kLevels.size(), static_cast<std::size_t>(5));
    RD_CHECK_EQ(BrightnessController::kLevels[0], 100);
    RD_CHECK_EQ(BrightnessController::kLevels[1], 75);
    RD_CHECK_EQ(BrightnessController::kLevels[2], 50);
    RD_CHECK_EQ(BrightnessController::kLevels[3], 25);
    RD_CHECK_EQ(BrightnessController::kLevels[4], 0);
}

RD_TEST(ui_brightness_steps_down_and_clamps) {
    BrightnessController brightness;
    RD_CHECK_EQ(brightness.uiPercent(), 100);

    brightness.uiDown();
    RD_CHECK_EQ(brightness.uiPercent(), 75);
    brightness.uiDown();
    RD_CHECK_EQ(brightness.uiPercent(), 50);
    brightness.uiDown();
    RD_CHECK_EQ(brightness.uiPercent(), 25);
    brightness.uiDown();
    RD_CHECK_EQ(brightness.uiPercent(), 0);
    brightness.uiDown();
    RD_CHECK_EQ(brightness.uiPercent(), 0);  // clamped

    brightness.uiUp();
    RD_CHECK_EQ(brightness.uiPercent(), 25);
    for (int i = 0; i < 10; ++i) brightness.uiUp();
    RD_CHECK_EQ(brightness.uiPercent(), 100);  // clamped
}

RD_TEST(ui_brightness_scales_theme_colours) {
    BrightnessController brightness;
    const Theme& base = retrodisc::defaultTheme();

    brightness.setUiLevel(0);
    const Theme full = brightness.apply(base);
    RD_CHECK_EQ(static_cast<int>(full.foreground.r), static_cast<int>(base.foreground.r));

    brightness.setUiLevel(1);  // 75 %
    const Theme dimmed = brightness.apply(base);
    RD_CHECK_EQ(static_cast<int>(dimmed.foreground.r),
                static_cast<int>(base.foreground.r) * 75 / 100);

    brightness.setUiLevel(4);  // 0 %
    const Theme blank = brightness.apply(base);
    RD_CHECK_EQ(static_cast<int>(blank.foreground.g), 0);
    RD_CHECK_EQ(static_cast<int>(blank.background.g), 0);
}

RD_TEST(display_off_theme_is_not_scaled) {
    BrightnessController brightness;
    const Theme& off = *retrodisc::themeById("off");
    brightness.setUiLevel(4);
    const Theme applied = brightness.apply(off);
    RD_CHECK(applied.displayOff);
    RD_CHECK_EQ(static_cast<int>(applied.foreground.r), 0);
}

RD_TEST(backlight_channel_is_independent_of_ui) {
    MockBacklight device(200);
    BrightnessController brightness(&device);

    RD_CHECK(brightness.backlightAvailable());
    brightness.setBacklightLevel(3);  // 25 %
    RD_CHECK_EQ(device.percent(), 25);
    RD_CHECK_EQ(device.raw(), 50);  // 25 % of 200

    brightness.setUiLevel(4);  // UI to black, backlight untouched
    RD_CHECK_EQ(device.percent(), 25);
}

RD_TEST(missing_backlight_degrades_gracefully) {
    NullBacklight none;
    BrightnessController brightness(&none);

    RD_CHECK(!brightness.backlightAvailable());
    RD_CHECK_EQ(brightness.backlightPercent(), 0);
    brightness.setBacklightLevel(2);  // must not crash or throw
    RD_CHECK_EQ(brightness.backlightPercent(), 0);
    RD_CHECK_EQ(brightness.uiPercent(), 100);  // UI channel still works
}

RD_TEST(controller_without_device_is_safe) {
    BrightnessController brightness(nullptr);
    RD_CHECK(!brightness.backlightAvailable());
    brightness.setBacklightLevel(1);
    brightness.uiDown();
    RD_CHECK_EQ(brightness.uiPercent(), 75);
}

RD_TEST(sysfs_backlight_absent_is_unavailable) {
    SysfsBacklight missing("/tmp/retrodisc_no_such_backlight_root");
    RD_CHECK(!missing.available());
    RD_CHECK(!missing.setPercent(50));
    RD_CHECK_EQ(missing.percent(), 0);
}

RD_TEST(mock_backlight_clamps) {
    MockBacklight device(100);
    RD_CHECK(device.setPercent(150));
    RD_CHECK_EQ(device.percent(), 100);
    RD_CHECK(device.setPercent(-20));
    RD_CHECK_EQ(device.percent(), 0);
}

RD_TEST(display_power_toggles) {
    DisplayPower power;
    RD_CHECK(power.on());
    power.toggle();
    RD_CHECK(!power.on());
    power.wake();
    RD_CHECK(power.on());
    power.sleep();
    RD_CHECK(!power.on());
}

RD_TEST(raw_values_round_trip_for_exact_restore) {
    MockBacklight device(120000);
    RD_CHECK(device.setRaw(37499));
    RD_CHECK_EQ(device.raw(), 37499);
    RD_CHECK_EQ(device.percent(), 31);  // 37499 / 120000

    RD_CHECK(device.setRaw(-10));
    RD_CHECK_EQ(device.raw(), 0);
    RD_CHECK(device.setRaw(999999));
    RD_CHECK_EQ(device.raw(), 120000);
}

RD_TEST(null_backlight_has_no_raw_control) {
    NullBacklight none;
    RD_CHECK_EQ(none.raw(), 0);
    RD_CHECK(!none.setRaw(100));
}

RD_TEST(controller_backlight_index_is_reported) {
    MockBacklight device(100);
    BrightnessController brightness(&device);
    RD_CHECK_EQ(brightness.uiLevelIndex(), 0);
    brightness.uiDown();
    brightness.uiDown();
    RD_CHECK_EQ(brightness.uiLevelIndex(), 2);
    brightness.setBacklightLevel(2);
    RD_CHECK_EQ(brightness.backlightLevelIndex(), 2);
    RD_CHECK_EQ(device.percent(), 50);
}

RD_TEST(zero_brightness_wakes_on_any_key) {
    MockBacklight device(100);
    BrightnessController brightness(&device);
    RD_CHECK(!brightness.wakeFromBlank());  // already visible

    brightness.setUiLevel(4);  // 0 %
    brightness.setBacklightLevel(4);
    RD_CHECK_EQ(brightness.uiPercent(), 0);

    RD_CHECK(brightness.wakeFromBlank());
    RD_CHECK_EQ(brightness.uiPercent(), 25);
    RD_CHECK_EQ(device.percent(), 25);

    // A second key is a normal key again.
    RD_CHECK(!brightness.wakeFromBlank());
}

RD_TEST_MAIN()
