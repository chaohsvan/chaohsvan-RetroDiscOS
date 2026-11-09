#include <map>
#include <string>

#include "core/config.h"
#include "input/key_mapping.h"
#include "test_util.h"

using retrodisc::Action;
using retrodisc::defaultConfig;
using retrodisc::KeyMap;
using retrodisc::normalizeKeyName;

RD_TEST(default_config_builds_a_complete_keymap) {
    const auto config = defaultConfig();
    const KeyMap keys(config.config.keys);

    RD_CHECK_EQ(keys.size(), config.config.keys.size());
    RD_CHECK(keys.resolve("SPACE") == Action::PlayPause);
    RD_CHECK(keys.resolve("RIGHT") == Action::Next);
    RD_CHECK(keys.resolve("LEFT") == Action::Prev);
    RD_CHECK(keys.resolve("F1") == Action::Mode);
    RD_CHECK(keys.resolve("F5") == Action::Theme);
    RD_CHECK(keys.resolve("E") == Action::Eject);
    RD_CHECK(keys.resolve("X") == Action::Stop);
}

RD_TEST(key_names_are_case_insensitive) {
    RD_CHECK_EQ(normalizeKeyName("space"), std::string("SPACE"));
    RD_CHECK_EQ(normalizeKeyName("Page Down"), std::string("PAGEDOWN"));
    RD_CHECK_EQ(normalizeKeyName("f12"), std::string("F12"));
}

RD_TEST(unknown_keys_resolve_to_none) {
    const KeyMap keys(defaultConfig().config.keys);
    RD_CHECK(keys.resolve("F13") == Action::None);
    RD_CHECK(keys.resolve("") == Action::None);
}

RD_TEST(custom_bindings_override_defaults) {
    std::map<std::string, std::string> bindings;
    bindings["play_pause"] = "ENTER";
    bindings["next"] = "n";
    const KeyMap keys(bindings);

    RD_CHECK(keys.resolve("ENTER") == Action::PlayPause);
    RD_CHECK(keys.resolve("N") == Action::Next);
    RD_CHECK(keys.resolve("SPACE") == Action::None);  // not bound in this map
    RD_CHECK_EQ(keys.size(), static_cast<std::size_t>(2));
}

RD_TEST(unknown_actions_are_ignored) {
    std::map<std::string, std::string> bindings;
    bindings["launch_rocket"] = "SPACE";
    bindings["next"] = "RIGHT";
    const KeyMap keys(bindings);

    RD_CHECK_EQ(keys.size(), static_cast<std::size_t>(1));
    RD_CHECK(keys.resolve("SPACE") == Action::None);
    RD_CHECK(keys.resolve("RIGHT") == Action::Next);
}

RD_TEST(action_names_round_trip) {
    RD_CHECK_EQ(std::string(retrodisc::actionName(Action::PlayPause)), std::string("play_pause"));
    RD_CHECK(retrodisc::actionFromName("play_pause") == Action::PlayPause);
    RD_CHECK(retrodisc::actionFromName("nonsense") == Action::None);
    RD_CHECK(retrodisc::actionFromName(retrodisc::actionName(Action::Eject)) == Action::Eject);
}

RD_TEST_MAIN()
