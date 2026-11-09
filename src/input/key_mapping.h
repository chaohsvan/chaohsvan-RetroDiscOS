#pragma once

#include <map>
#include <string>
#include <string_view>

namespace retrodisc {

// Semantic actions. The UI never looks at raw keys, and the input layer never
// looks at the UI (AGENTS.md 5 / D3).
enum class Action {
    None,
    PlayPause,
    Stop,
    Next,
    Prev,
    SeekBack,
    SeekFwd,
    VolUp,
    VolDown,
    Mute,
    Mode,
    Random,
    Repeat,
    Info,
    Theme,
    BrightnessDown,
    BrightnessUp,
    DisplayToggle,
    Spectrum,
    Eject,
    Help,
    Service,
    Quit,
};

std::string_view actionName(Action action);
Action actionFromName(std::string_view name);

// Maps normalized key names ("SPACE", "RIGHT", "F1") to actions. Built from
// config/keys.conf so remapping never requires a rebuild.
class KeyMap {
   public:
    KeyMap() = default;
    explicit KeyMap(const std::map<std::string, std::string>& keys);

    Action resolve(std::string_view keyName) const;
    bool empty() const {
        return bindings_.empty();
    }
    std::size_t size() const {
        return bindings_.size();
    }

   private:
    std::map<std::string, Action> bindings_;
};

std::string normalizeKeyName(std::string_view name);

}  // namespace retrodisc
