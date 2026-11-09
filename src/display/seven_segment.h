#pragma once

#include <string_view>

struct SDL_Renderer;

namespace retrodisc {

// Seven-segment digits for the VFD theme. Drawn as rectangles so it scales to
// any panel size without a font.
void drawSevenSegmentGlyph(SDL_Renderer* renderer, int x, int y, int width, int height,
                           int thickness, char glyph);

// Draws "03:45" style text. Returns the width used.
int drawSevenSegmentText(SDL_Renderer* renderer, int x, int y, int digitWidth, int digitHeight,
                         int thickness, std::string_view text);

int sevenSegmentTextWidth(int digitWidth, int thickness, std::string_view text);

}  // namespace retrodisc
