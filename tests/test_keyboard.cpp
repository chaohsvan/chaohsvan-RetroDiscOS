#include <linux/input-event-codes.h>

#include <string>

#include "core/config.h"
#include "input/evdev_keyboard.h"
#include "input/key_mapping.h"
#include "input/key_names.h"
#include "test_util.h"

using retrodisc::Action;
using retrodisc::EvdevKeyboard;
using retrodisc::keyCodeFromName;
using retrodisc::KeyMap;
using retrodisc::keyNameFromCode;

RD_TEST(evdev_codes_map_to_config_names) {
    RD_CHECK_EQ(keyNameFromCode(KEY_SPACE), std::string("SPACE"));
    RD_CHECK_EQ(keyNameFromCode(KEY_LEFT), std::string("LEFT"));
    RD_CHECK_EQ(keyNameFromCode(KEY_RIGHT), std::string("RIGHT"));
    RD_CHECK_EQ(keyNameFromCode(KEY_UP), std::string("UP"));
    RD_CHECK_EQ(keyNameFromCode(KEY_DOWN), std::string("DOWN"));
    RD_CHECK_EQ(keyNameFromCode(KEY_ENTER), std::string("ENTER"));
    RD_CHECK_EQ(keyNameFromCode(KEY_F5), std::string("F5"));
    RD_CHECK_EQ(keyNameFromCode(KEY_E), std::string("E"));
    RD_CHECK_EQ(keyNameFromCode(KEY_Q), std::string("Q"));
}

RD_TEST(names_map_back_to_the_same_codes) {
    const int codes[] = {KEY_SPACE, KEY_LEFT, KEY_RIGHT, KEY_F1, KEY_F12, KEY_E, KEY_Q};
    for (int code : codes) {
        const std::string name = keyNameFromCode(code);
        RD_CHECK(!name.empty());
        RD_CHECK_EQ(keyCodeFromName(name), code);
    }
}

RD_TEST(name_lookup_is_case_insensitive) {
    RD_CHECK_EQ(keyCodeFromName("space"), KEY_SPACE);
    RD_CHECK_EQ(keyCodeFromName("Space"), KEY_SPACE);
    RD_CHECK_EQ(keyCodeFromName("f5"), KEY_F5);
}

RD_TEST(unknown_names_have_no_code) {
    RD_CHECK_EQ(keyCodeFromName("NOT_A_KEY"), 0);
    RD_CHECK_EQ(keyCodeFromName(""), 0);
    RD_CHECK(keyNameFromCode(0xFFFF).empty());
}

RD_TEST(keymap_resolves_evdev_names) {
    const KeyMap keys(retrodisc::defaultConfig().config.keys);
    RD_CHECK(keys.resolve(keyNameFromCode(KEY_SPACE)) == Action::PlayPause);
    RD_CHECK(keys.resolve(keyNameFromCode(KEY_RIGHT)) == Action::Next);
    RD_CHECK(keys.resolve(keyNameFromCode(KEY_F1)) == Action::Mode);
    RD_CHECK(keys.resolve(keyNameFromCode(KEY_F5)) == Action::Theme);
    RD_CHECK(keys.resolve(keyNameFromCode(KEY_E)) == Action::Eject);
    RD_CHECK(keys.resolve(keyNameFromCode(KEY_F13)) == Action::None);
}

RD_TEST(opening_a_missing_device_fails_cleanly) {
    const KeyMap keys(retrodisc::defaultConfig().config.keys);
    EvdevKeyboard keyboard(keys);
    RD_CHECK(!keyboard.open("/dev/input/retrodisc-does-not-exist"));
    RD_CHECK(!keyboard.isOpen());
    RD_CHECK(!keyboard.lastError().empty());
    RD_CHECK(keyboard.poll(0) == Action::None);
}

RD_TEST(non_evdev_file_is_rejected) {
    const KeyMap keys(retrodisc::defaultConfig().config.keys);
    EvdevKeyboard keyboard(keys);
    RD_CHECK(!keyboard.open("/dev/null"));
    RD_CHECK(!keyboard.isOpen());
    RD_CHECK(!keyboard.lastError().empty());
}

RD_TEST(polling_a_closed_device_is_harmless) {
    const KeyMap keys(retrodisc::defaultConfig().config.keys);
    EvdevKeyboard keyboard(keys);
    RD_CHECK(keyboard.poll(0) == Action::None);
    keyboard.close();
    RD_CHECK(!keyboard.isOpen());
}

RD_TEST_MAIN()
