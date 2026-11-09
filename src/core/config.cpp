#include "core/config.h"

#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>

namespace retrodisc {
namespace {

constexpr float kDefaultVolume = 0.8f;

const std::pair<std::string_view, std::string_view> kDefaultKeys[] = {
    {"play_pause", "SPACE"}, {"stop", "X"},
    {"next", "RIGHT"},       {"prev", "LEFT"},
    {"vol_up", "UP"},        {"vol_down", "DOWN"},
    {"mode", "F1"},          {"random", "F2"},
    {"repeat", "F3"},        {"info", "F4"},
    {"theme", "F5"},         {"brightness_down", "F6"},
    {"brightness_up", "F7"}, {"display_toggle", "F8"},
    {"spectrum", "F9"},      {"mute", "F10"},
    {"seek_back", "F11"},    {"seek_fwd", "F12"},
    {"eject", "E"},          {"help", "H"},
    {"service", "S"},        {"quit", "Q"},
};

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// Trailing comments are allowed ("theme = amber  # note"), but a '#' without a
// preceding space is kept as part of the value.
std::string stripTrailingComment(const std::string& text) {
    const auto pos = text.find(" #");
    if (pos == std::string::npos) return text;
    return trim(text.substr(0, pos));
}

std::string toUpper(std::string text) {
    for (char& c : text) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    return text;
}

void warn(ConfigResult& result, const std::string& path, int line, const std::string& message) {
    std::ostringstream out;
    out << path << ":" << line << ": " << message;
    result.warnings.push_back(out.str());
}

bool parseVolume(const std::string& text, float* out) {
    if (text.empty()) return false;
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0') return false;
    *out = value;
    return true;
}

void applyPlayerEntry(ConfigResult& result, const std::string& path, int line,
                      const std::string& key, const std::string& value) {
    if (key == "audio_device") {
        if (value.empty()) {
            warn(result, path, line, "audio_device 不能为空，已忽略");
            return;
        }
        result.config.audioDevice = value;
    } else if (key == "volume") {
        float parsed = 0.0f;
        if (!parseVolume(value, &parsed)) {
            warn(result, path, line, "volume 不是合法数字，已忽略: " + value);
            return;
        }
        if (parsed < 0.0f || parsed > 1.0f) {
            warn(result, path, line, "volume 超出 [0,1]，已钳制: " + value);
            parsed = parsed < 0.0f ? 0.0f : 1.0f;
        }
        result.config.volume = parsed;
    } else if (key == "theme") {
        if (value.empty()) {
            warn(result, path, line, "theme 不能为空，已忽略");
            return;
        }
        result.config.theme = value;
    } else if (key == "resume_file") {
        result.config.resumeFile = value;
    } else {
        warn(result, path, line, "未知配置项，已忽略: " + key);
    }
}

void applyKeyEntry(ConfigResult& result, const std::string& path, int line,
                   const std::string& action, const std::string& value,
                   std::set<std::string>& seenActions) {
    if (!isKnownAction(action)) {
        warn(result, path, line, "未知按键动作，已忽略: " + action);
        return;
    }
    if (value.empty()) {
        warn(result, path, line, "按键值不能为空，已忽略: " + action);
        return;
    }
    if (seenActions.count(action) != 0 && result.config.keys[action] != toUpper(value)) {
        warn(result, path, line, "按键动作重复定义，后值覆盖前值: " + action);
    }
    seenActions.insert(action);
    result.config.keys[action] = toUpper(value);
}

void loadOneFile(ConfigResult& result, const std::string& path, bool keysFile) {
    std::ifstream in(path);
    if (!in.is_open()) {
        warn(result, path, 0, "配置文件不存在，使用默认值");
        return;
    }

    std::set<std::string> seenActions;
    std::string rawLine;
    int lineNumber = 0;
    while (std::getline(in, rawLine)) {
        ++lineNumber;
        const std::string line = trim(rawLine);
        if (line.empty() || line[0] == '#') continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) {
            warn(result, path, lineNumber, "缺少 '='，已跳过: " + line);
            continue;
        }

        const std::string key = stripTrailingComment(trim(line.substr(0, eq)));
        const std::string value = stripTrailingComment(trim(line.substr(eq + 1)));
        if (key.empty()) {
            warn(result, path, lineNumber, "键名为空，已跳过");
            continue;
        }

        if (keysFile) {
            applyKeyEntry(result, path, lineNumber, key, value, seenActions);
        } else {
            applyPlayerEntry(result, path, lineNumber, key, value);
        }
    }
}

}  // namespace

bool isKnownAction(std::string_view action) {
    for (const auto& entry : kDefaultKeys) {
        if (entry.first == action) return true;
    }
    return false;
}

std::string defaultKey(std::string_view action) {
    for (const auto& entry : kDefaultKeys) {
        if (entry.first == action) return std::string(entry.second);
    }
    return {};
}

ConfigResult defaultConfig() {
    ConfigResult result;
    result.config.audioDevice = "null";
    result.config.volume = kDefaultVolume;
    result.config.theme = "digital-hifi";
    result.config.resumeFile.clear();
    result.config.keys.clear();
    for (const auto& entry : kDefaultKeys) {
        result.config.keys[std::string(entry.first)] = std::string(entry.second);
    }
    return result;
}

ConfigResult loadConfigFiles(const std::string& playerPath, const std::string& keysPath) {
    ConfigResult result = defaultConfig();
    loadOneFile(result, playerPath, false);
    loadOneFile(result, keysPath, true);
    return result;
}

ConfigResult loadConfig(const std::string& directory) {
    const std::string base = directory.empty() ? std::string("config") : directory;
    return loadConfigFiles(base + "/player.conf", base + "/keys.conf");
}

}  // namespace retrodisc
