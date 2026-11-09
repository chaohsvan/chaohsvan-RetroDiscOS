#pragma once

struct SDL_Renderer;

namespace retrodisc {

struct Theme;

// Needs VU meter for the vintage-analog theme. left/right are 0..1 levels.
void drawVuMeter(SDL_Renderer* renderer, int x, int y, int width, int height, float left,
                 float right, const Theme& theme);

}  // namespace retrodisc
