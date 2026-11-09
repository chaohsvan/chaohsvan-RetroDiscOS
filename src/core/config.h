#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace retrodisc {

// All tunables live here; code only carries defaults (AGENTS.md 5.5).
struct PlayerConfig {
    std::string audioDevice = "null";
    float volume = 0.8f;
    std::string theme = "digital-hifi";
    std::string resumeFile;
    std::map<std::string, std::string> keys;  // action -> key name
};

struct ConfigResult {
    PlayerConfig config;
    std::vector<std::string> warnings;
    bool ok = true;
};

// Built-in defaults, used when the config directory is missing or incomplete.
ConfigResult defaultConfig();

// Reads <directory>/player.conf and <directory>/keys.conf. Missing files are
// reported as warnings and fall back to defaults rather than failing.
ConfigResult loadConfig(const std::string& directory);

// Same, with explicit paths (used by tests).
ConfigResult loadConfigFiles(const std::string& playerPath, const std::string& keysPath);

bool isKnownAction(std::string_view action);
std::string defaultKey(std::string_view action);

}  // namespace retrodisc
