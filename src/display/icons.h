#pragma once

struct SDL_Renderer;
struct SDL_Color;

namespace retrodisc {

// Transport icons and the spinning disc, all drawn with primitives so no image
// assets or extra dependencies are needed.
void drawPlayIcon(SDL_Renderer* renderer, int x, int y, int size, SDL_Color color);
void drawPauseIcon(SDL_Renderer* renderer, int x, int y, int size, SDL_Color color);
void drawStopIcon(SDL_Renderer* renderer, int x, int y, int size, SDL_Color color);

// angleDeg rotates the marker line; callers pass a time-derived angle.
void drawDisc(SDL_Renderer* renderer, int centerX, int centerY, int radius, double angleDeg,
              SDL_Color rim, SDL_Color marker);

}  // namespace retrodisc
