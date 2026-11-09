#pragma once

#include <string_view>

#include "display/font5x7.h"

struct SDL_Renderer;

namespace retrodisc {

// Width of one glyph cell including the 1px inter-glyph gap.
inline constexpr int kGlyphAdvance = kGlyphWidth + 1;

int textWidth(std::string_view text, int scale);
int textHeight(int scale);

// Draws upper-case ASCII with the built-in 5x7 font. Lower-case is upcased;
// unknown characters render as blanks.
void drawText(SDL_Renderer* renderer, int x, int y, int scale, std::string_view text);

// Truncates text so that textWidth(result, scale) <= maxWidth.
std::string_view fitText(std::string_view text, int scale, int maxWidth);

}  // namespace retrodisc
