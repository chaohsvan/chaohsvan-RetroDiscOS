#pragma once

#include <array>
#include <cstddef>

#include "display/backlight.h"
#include "display/theme.h"

namespace retrodisc {

// UI brightness and hardware backlight are two independent channels
// (PRD "亮度管理"). UI brightness scales rendered colours; the backlight is a
// physical device that may be missing entirely.
class BrightnessController {
   public:
    static constexpr std::array<int, 5> kLevels{100, 75, 50, 25, 0};

    explicit BrightnessController(IBacklightDevice* backlight = nullptr);

    int uiPercent() const {
        return kLevels[uiIndex_];
    }
    int backlightPercent() const;
    bool backlightAvailable() const;

    // Level index in [0, kLevels.size()); clamped.
    void setUiLevel(int index);
    void setBacklightLevel(int index);
    void uiUp();
    void uiDown();

    // Applies the UI brightness to a theme's colours.
    Theme apply(const Theme& theme) const;

   private:
    static std::size_t clampIndex(int index);

    IBacklightDevice* backlight_ = nullptr;
    std::size_t uiIndex_ = 0;
    std::size_t backlightIndex_ = 0;
};

}  // namespace retrodisc
