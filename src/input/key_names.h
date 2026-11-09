#pragma once

#include <string>
#include <string_view>

namespace retrodisc {

// Translates between Linux evdev key codes and the names used in config/keys.conf
// ("SPACE", "RIGHT", "F5", "E"). libevdev reports names like "KEY_SPACE", and the
// "KEY_" prefix is stripped here so one vocabulary covers both input paths.
std::string keyNameFromCode(int evdevCode);

// Returns 0 when the name is unknown (evdev has no key code 0).
int keyCodeFromName(std::string_view name);

}  // namespace retrodisc
