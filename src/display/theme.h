#pragma once

#include <SDL2/SDL.h>

#include <string>
#include <string_view>
#include <vector>

namespace retrodisc {

enum class ClockStyle {
    Bitmap,        // 5x7 dot matrix
    SevenSegment,  // segmented VFD digits
};

// A display theme is pure data: colours plus a flag for the "display off" mode.
// Layout lives in the UI layer; themes must not contain drawing code.
struct Theme {
    std::string id;
    std::string name;
    SDL_Color background;
    SDL_Color foreground;
    SDL_Color dim;
    SDL_Color accent;
    bool displayOff = false;
    ClockStyle clockStyle = ClockStyle::Bitmap;
    bool vuMeter = false;
};

const std::vector<Theme>& allThemes();
const Theme& defaultTheme();
const Theme* themeById(std::string_view id);
const Theme& themeOrFallback(std::string_view id);

// Cycles through the registry order; unknown ids start at the default theme.
std::string nextThemeId(std::string_view id);

}  // namespace retrodisc
