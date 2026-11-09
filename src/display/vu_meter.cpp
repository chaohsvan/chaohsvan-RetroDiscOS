#include "display/vu_meter.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>

#include "display/text_renderer.h"
#include "display/theme.h"

namespace retrodisc {
namespace {

void setColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

// Rotates a needle around (cx, cy); angle 0 is straight up, positive goes right.
void drawNeedle(SDL_Renderer* renderer, int cx, int cy, int length, double angleDeg,
                SDL_Color color) {
    setColor(renderer, color);
    const double rad = angleDeg * M_PI / 180.0;
    const double dx = std::sin(rad);
    const double dy = -std::cos(rad);
    for (int i = 0; i < length; ++i) {
        const int px = cx + static_cast<int>(dx * i);
        const int py = cy + static_cast<int>(dy * i);
        const SDL_Rect dot{px, py, 2, 2};
        SDL_RenderFillRect(renderer, &dot);
    }
}

}  // namespace

void drawVuMeter(SDL_Renderer* renderer, int x, int y, int width, int height, float left,
                 float right, const Theme& theme) {
    if (renderer == nullptr || width <= 0 || height <= 0) return;

    // Face plate
    setColor(renderer, theme.background);
    const SDL_Rect face{x, y, width, height};
    SDL_RenderFillRect(renderer, &face);
    setColor(renderer, theme.dim);
    SDL_RenderDrawRect(renderer, &face);

    const int cx = x + width / 2;
    const int cy = y + height - std::max(4, height / 12);
    const int radius = std::min(width / 2 - 6, height - 10);
    if (radius <= 4) return;

    // Tick marks every 10 %, from -45 to +45 degrees.
    const double startAngle = -45.0;
    const double endAngle = 45.0;
    for (int tick = 0; tick <= 10; ++tick) {
        const double t = static_cast<double>(tick) / 10.0;
        const double angle = startAngle + (endAngle - startAngle) * t;
        const double rad = angle * M_PI / 180.0;
        const int inner = (tick % 5 == 0) ? radius - 10 : radius - 6;
        for (int r = inner; r < radius; ++r) {
            const int px = cx + static_cast<int>(std::sin(rad) * r);
            const int py = cy - static_cast<int>(std::cos(rad) * r);
            const SDL_Rect dot{px, py, 2, 2};
            setColor(renderer, tick >= 8 ? theme.accent : theme.dim);
            SDL_RenderFillRect(renderer, &dot);
        }
    }

    const auto levelToAngle = [&](float level) {
        const float clamped = std::clamp(level, 0.0f, 1.0f);
        return startAngle + (endAngle - startAngle) * static_cast<double>(clamped);
    };

    drawNeedle(renderer, cx, cy, radius - 4, levelToAngle(left) - 6.0, theme.foreground);
    drawNeedle(renderer, cx, cy, radius - 4, levelToAngle(right) + 6.0, theme.accent);

    // Labels
    const int scale = std::max(2, height / 22);
    setColor(renderer, theme.dim);
    drawText(renderer, x + 4, y + 4, scale, "L");
    const int rightX = x + width - 4 - textWidth("R", scale);
    drawText(renderer, rightX, y + 4, scale, "R");
}

}  // namespace retrodisc
