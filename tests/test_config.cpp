#include <cstdio>
#include <fstream>
#include <string>

#include "core/config.h"
#include "test_util.h"

using retrodisc::ConfigResult;
using retrodisc::defaultConfig;
using retrodisc::isKnownAction;
using retrodisc::loadConfig;
using retrodisc::loadConfigFiles;

namespace {

std::string writeTemp(const char* name, const std::string& content) {
    const std::string path = std::string("/tmp/retrodisc_cfg_") + name;
    std::ofstream out(path, std::ios::trunc);
    out << content;
    out.close();
    return path;
}

int warningsMentioning(const ConfigResult& result, const std::string& needle) {
    int count = 0;
    for (const auto& warning : result.warnings) {
        if (warning.find(needle) != std::string::npos) ++count;
    }
    return count;
}

}  // namespace

RD_TEST(defaults_are_complete) {
    const ConfigResult result = defaultConfig();
    RD_CHECK(result.ok);
    RD_CHECK(result.warnings.empty());
    RD_CHECK_EQ(result.config.audioDevice, std::string("null"));
    RD_CHECK(result.config.volume > 0.79f && result.config.volume < 0.81f);
    RD_CHECK_EQ(result.config.theme, std::string("digital-hifi"));
    RD_CHECK_EQ(result.config.keys.size(), static_cast<std::size_t>(22));
    RD_CHECK_EQ(result.config.keys.at("play_pause"), std::string("SPACE"));
    RD_CHECK_EQ(result.config.keys.at("next"), std::string("RIGHT"));
}

RD_TEST(missing_files_fall_back_to_defaults) {
    const ConfigResult result = loadConfigFiles("/tmp/retrodisc_does_not_exist_player.conf",
                                                "/tmp/retrodisc_does_not_exist_keys.conf");
    RD_CHECK(result.ok);
    RD_CHECK_EQ(static_cast<int>(result.warnings.size()), 2);
    RD_CHECK_EQ(result.config.audioDevice, std::string("null"));
    RD_CHECK_EQ(result.config.keys.size(), static_cast<std::size_t>(22));
}

RD_TEST(player_conf_overrides_defaults) {
    const std::string player = writeTemp("player_ok.conf",
                                         "audio_device = hw:1,0\n"
                                         "volume = 0.35\n"
                                         "theme = amber\n"
                                         "resume_file = /tmp/resume.json\n");
    const std::string keys = writeTemp("keys_ok.conf", "play_pause = ENTER\n");

    const ConfigResult result = loadConfigFiles(player, keys);
    RD_CHECK(result.warnings.empty());
    RD_CHECK_EQ(result.config.audioDevice, std::string("hw:1,0"));
    RD_CHECK(result.config.volume > 0.34f && result.config.volume < 0.36f);
    RD_CHECK_EQ(result.config.theme, std::string("amber"));
    RD_CHECK_EQ(result.config.resumeFile, std::string("/tmp/resume.json"));
    RD_CHECK_EQ(result.config.keys.at("play_pause"), std::string("ENTER"));
}

RD_TEST(volume_out_of_range_is_clamped_and_warned) {
    const std::string player = writeTemp("player_volume.conf", "volume = 2.5\n");
    const ConfigResult high = loadConfigFiles(player, "/tmp/retrodisc_none_keys.conf");
    RD_CHECK(high.config.volume == 1.0f);
    RD_CHECK(warningsMentioning(high, "volume") > 0);

    const std::string player2 = writeTemp("player_volume2.conf", "volume = -1\n");
    const ConfigResult low = loadConfigFiles(player2, "/tmp/retrodisc_none_keys.conf");
    RD_CHECK(low.config.volume == 0.0f);
    RD_CHECK(warningsMentioning(low, "volume") > 0);
}

RD_TEST(volume_garbage_keeps_default) {
    const std::string player = writeTemp("player_bad_volume.conf", "volume = loud\n");
    const ConfigResult result = loadConfigFiles(player, "/tmp/retrodisc_none_keys.conf");
    RD_CHECK(result.config.volume > 0.79f && result.config.volume < 0.81f);
    RD_CHECK(warningsMentioning(result, "volume") > 0);
}

RD_TEST(unknown_player_key_is_warned_but_others_apply) {
    const std::string player = writeTemp("player_unknown.conf",
                                         "colour = blue\n"
                                         "theme = amber\n");
    const ConfigResult result = loadConfigFiles(player, "/tmp/retrodisc_none_keys.conf");
    RD_CHECK_EQ(result.config.theme, std::string("amber"));
    RD_CHECK(warningsMentioning(result, "colour") > 0);
}

RD_TEST(malformed_lines_are_skipped) {
    const std::string player = writeTemp("player_malformed.conf",
                                         "this line has no equals sign\n"
                                         "volume = 0.4\n"
                                         "= orphan value\n");
    const ConfigResult result = loadConfigFiles(player, "/tmp/retrodisc_none_keys.conf");
    RD_CHECK(result.config.volume > 0.39f && result.config.volume < 0.41f);
    RD_CHECK_EQ(static_cast<int>(result.warnings.size()), 3);  // 2 bad lines + missing keys.conf
}

RD_TEST(duplicate_key_last_one_wins) {
    const std::string keys = writeTemp("keys_dup.conf",
                                       "next = RIGHT\n"
                                       "next = PAGE_DOWN\n");
    const ConfigResult result = loadConfigFiles("/tmp/retrodisc_none_player.conf", keys);
    RD_CHECK_EQ(result.config.keys.at("next"), std::string("PAGE_DOWN"));
    RD_CHECK(warningsMentioning(result, "重复") > 0);
}

RD_TEST(key_values_are_normalized_to_upper_case) {
    const std::string keys = writeTemp("keys_case.conf", "play_pause = space\n");
    const ConfigResult result = loadConfigFiles("/tmp/retrodisc_none_player.conf", keys);
    RD_CHECK_EQ(result.config.keys.at("play_pause"), std::string("SPACE"));
}

RD_TEST(unknown_action_is_warned) {
    const std::string keys = writeTemp("keys_unknown.conf", "launch_rocket = SPACE\n");
    const ConfigResult result = loadConfigFiles("/tmp/retrodisc_none_player.conf", keys);
    RD_CHECK(warningsMentioning(result, "launch_rocket") > 0);
    RD_CHECK(!isKnownAction("launch_rocket"));
    RD_CHECK(isKnownAction("play_pause"));
}

RD_TEST(comments_and_blank_lines_are_ignored) {
    const std::string player = writeTemp("player_comments.conf",
                                         "# a comment\n"
                                         "\n"
                                         "   \n"
                                         "theme = amber  # trailing text is not special\n");
    const ConfigResult result = loadConfigFiles(player, "/tmp/retrodisc_none_keys.conf");
    RD_CHECK(warningsMentioning(result, "theme") == 0);
    RD_CHECK_EQ(result.config.theme, std::string("amber"));
}

RD_TEST(shipped_config_files_are_clean) {
    const ConfigResult result = loadConfig(RETRODISC_CONFIG_DIR);
    RD_CHECK(result.ok);
    RD_CHECK(result.warnings.empty());
    RD_CHECK_EQ(result.config.keys.size(), static_cast<std::size_t>(22));
    RD_CHECK_EQ(result.config.keys.at("stop"), std::string("X"));
    RD_CHECK_EQ(result.config.theme, std::string("digital-hifi"));
}

RD_TEST_MAIN()
