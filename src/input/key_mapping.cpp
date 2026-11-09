#include "input/key_mapping.h"

#include <array>
#include <utility>

namespace retrodisc {
namespace {

constexpr std::pair<Action, std::string_view> kActionNames[] = {
    {Action::None, "none"},
    {Action::PlayPause, "play_pause"},
    {Action::Stop, "stop"},
    {Action::Next, "next"},
    {Action::Prev, "prev"},
    {Action::SeekBack, "seek_back"},
    {Action::SeekFwd, "seek_fwd"},
    {Action::VolUp, "vol_up"},
    {Action::VolDown, "vol_down"},
    {Action::Mute, "mute"},
    {Action::Mode, "mode"},
    {Action::Random, "random"},
    {Action::Repeat, "repeat"},
    {Action::Info, "info"},
    {Action::Theme, "theme"},
    {Action::BrightnessDown, "brightness_down"},
    {Action::BrightnessUp, "brightness_up"},
    {Action::DisplayToggle, "display_toggle"},
    {Action::Spectrum, "spectrum"},
    {Action::Eject, "eject"},
    {Action::Help, "help"},
    {Action::Service, "service"},
    {Action::Quit, "quit"},
};

}  // namespace

std::string normalizeKeyName(std::string_view name) {
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if (c >= 'a' && c <= 'z') {
            out.push_back(static_cast<char>(c - 'a' + 'A'));
        } else if (c != ' ' && c != '\t') {
            out.push_back(c);
        }
    }
    return out;
}

std::string_view actionName(Action action) {
    for (const auto& entry : kActionNames) {
        if (entry.first == action) return entry.second;
    }
    return "none";
}

Action actionFromName(std::string_view name) {
    for (const auto& entry : kActionNames) {
        if (entry.second == name) return entry.first;
    }
    return Action::None;
}

KeyMap::KeyMap(const std::map<std::string, std::string>& keys) {
    for (const auto& entry : keys) {
        const Action action = actionFromName(entry.first);
        if (action == Action::None) continue;  // unknown action: config already warned
        bindings_[normalizeKeyName(entry.second)] = action;
    }
}

Action KeyMap::resolve(std::string_view keyName) const {
    const auto it = bindings_.find(normalizeKeyName(keyName));
    return it == bindings_.end() ? Action::None : it->second;
}

}  // namespace retrodisc
