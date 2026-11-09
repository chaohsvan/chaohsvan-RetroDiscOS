#include "display/brightness.h"

#include <algorithm>

namespace retrodisc {
namespace {

Uint8 scaleChannel(Uint8 channel, int percent) {
    const int value = static_cast<int>(channel) * percent / 100;
    return static_cast<Uint8>(std::clamp(value, 0, 255));
}

}  // namespace

BrightnessController::BrightnessController(IBacklightDevice* backlight) : backlight_(backlight) {}

std::size_t BrightnessController::clampIndex(int index) {
    if (index < 0) return 0;
    const int last = static_cast<int>(kLevels.size()) - 1;
    if (index > last) return static_cast<std::size_t>(last);
    return static_cast<std::size_t>(index);
}

int BrightnessController::backlightPercent() const {
    if (backlight_ == nullptr || !backlight_->available()) return 0;
    return backlight_->percent();
}

bool BrightnessController::backlightAvailable() const {
    return backlight_ != nullptr && backlight_->available();
}

void BrightnessController::setUiLevel(int index) {
    uiIndex_ = clampIndex(index);
}

void BrightnessController::setBacklightLevel(int index) {
    backlightIndex_ = clampIndex(index);
    if (backlight_ != nullptr && backlight_->available()) {
        backlight_->setPercent(kLevels[backlightIndex_]);
    }
}

void BrightnessController::uiUp() {
    if (uiIndex_ > 0) --uiIndex_;
}

void BrightnessController::uiDown() {
    if (uiIndex_ + 1 < kLevels.size()) ++uiIndex_;
}

Theme BrightnessController::apply(const Theme& theme) const {
    if (theme.displayOff) return theme;

    const int percent = uiPercent();
    Theme out = theme;
    out.background = SDL_Color{scaleChannel(theme.background.r, percent),
                               scaleChannel(theme.background.g, percent),
                               scaleChannel(theme.background.b, percent), 255};
    out.foreground = SDL_Color{scaleChannel(theme.foreground.r, percent),
                               scaleChannel(theme.foreground.g, percent),
                               scaleChannel(theme.foreground.b, percent), 255};
    out.dim = SDL_Color{scaleChannel(theme.dim.r, percent), scaleChannel(theme.dim.g, percent),
                        scaleChannel(theme.dim.b, percent), 255};
    out.accent =
        SDL_Color{scaleChannel(theme.accent.r, percent), scaleChannel(theme.accent.g, percent),
                  scaleChannel(theme.accent.b, percent), 255};
    return out;
}

}  // namespace retrodisc
