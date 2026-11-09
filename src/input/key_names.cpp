#include "input/key_names.h"

#include <libevdev/libevdev.h>
#include <linux/input-event-codes.h>

#include <cctype>
#include <map>

namespace retrodisc {
namespace {

std::string normalize(std::string_view name) {
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if (c == ' ' || c == '\t') continue;
        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return out;
}

const std::map<std::string, int>& nameToCode() {
    static const std::map<std::string, int> table = [] {
        std::map<std::string, int> map;
        for (int code = 0; code <= KEY_MAX; ++code) {
            const char* raw = libevdev_event_code_get_name(EV_KEY, code);
            if (raw == nullptr) continue;
            const std::string name = normalize(raw);
            const std::string stripped = name.rfind("KEY_", 0) == 0 ? name.substr(4) : name;
            if (!stripped.empty()) map.emplace(stripped, code);
        }
        return map;
    }();
    return table;
}

}  // namespace

std::string keyNameFromCode(int evdevCode) {
    const char* raw = libevdev_event_code_get_name(EV_KEY, evdevCode);
    if (raw == nullptr) return {};
    std::string name = normalize(raw);
    if (name.rfind("KEY_", 0) == 0) name = name.substr(4);
    return name;
}

int keyCodeFromName(std::string_view name) {
    const auto it = nameToCode().find(normalize(name));
    return it == nameToCode().end() ? 0 : it->second;
}

}  // namespace retrodisc
