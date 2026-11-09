#include "display/icons.h"

#include <SDL2/SDL.h>

#include <cmath>

namespace retrodisc {
namespace {

void setColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void fillRect(SDL_Renderer* renderer, int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    const SDL_Rect rect{x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

}  // namespace

void drawPlayIcon(SDL_Renderer* renderer, int x, int y, int size, SDL_Color color) {
    if (renderer == nullptr || size <= 2) return;
    setColor(renderer, color);
    // Rows get wider as the triangle grows: a filled right-pointing wedge.
    for (int row = 0; row < size; ++row) {
        const int half = size / 2;
        const int distance = std::abs(row - half);
        const int width = size - distance * 2;
        if (width <= 0) continue;
        fillRect(renderer, x, y + row, width, 1);
    }
}

void drawPauseIcon(SDL_Renderer* renderer, int x, int y, int size, SDL_Color color) {
    if (renderer == nullptr || size <= 2) return;
    setColor(renderer, color);
    const int barWidth = size / 3;
    fillRect(renderer, x, y, barWidth, size);
    fillRect(renderer, x + size - barWidth, y, barWidth, size);
}

void drawStopIcon(SDL_Renderer* renderer, int x, int y, int size, SDL_Color color) {
    if (renderer == nullptr || size <= 2) return;
    setColor(renderer, color);
    fillRect(renderer, x, y, size, size);
}

void drawDisc(SDL_Renderer* renderer, int centerX, int centerY, int radius, double angleDeg,
              SDL_Color rim, SDL_Color marker) {
    if (renderer == nullptr || radius <= 4) return;

    // Rim: a circle of 2x2 dots, cheap and legible at any size.
    setColor(renderer, rim);
    for (int angle = 0; angle < 360; angle += 2) {
        const double rad = angle * M_PI / 180.0;
        const int px = centerX + static_cast<int>(std::cos(rad) * radius);
        const int py = centerY + static_cast<int>(std::sin(rad) * radius);
        fillRect(renderer, px, py, 2, 2);
    }

    // Spindle hole.
    for (int angle = 0; angle < 360; angle += 6) {
        const double rad = angle * M_PI / 180.0;
        const int px = centerX + static_cast<int>(std::cos(rad) * (radius / 5 + 1));
        const int py = centerY + static_cast<int>(std::sin(rad) * (radius / 5 + 1));
        fillRect(renderer, px, py, 2, 2);
    }

    // Rotating marker so the spin is visible.
    setColor(renderer, marker);
    const double rad = angleDeg * M_PI / 180.0;
    for (int r = radius / 4; r < radius - 2; ++r) {
        const int px = centerX + static_cast<int>(std::cos(rad) * r);
        const int py = centerY + static_cast<int>(std::sin(rad) * r);
        fillRect(renderer, px, py, 2, 2);
    }
}

}  // namespace retrodisc
