#include "display/seven_segment.h"

#include <SDL2/SDL.h>

#include <cstdint>

namespace retrodisc {
namespace {

// Segment bits: a=1, b=2, c=4, d=8, e=16, f=32, g=64 (the usual layout).
constexpr uint8_t kA = 1 << 0;
constexpr uint8_t kB = 1 << 1;
constexpr uint8_t kC = 1 << 2;
constexpr uint8_t kD = 1 << 3;
constexpr uint8_t kE = 1 << 4;
constexpr uint8_t kF = 1 << 5;
constexpr uint8_t kG = 1 << 6;

uint8_t segmentsFor(char glyph) {
    switch (glyph) {
        case '0': return kA | kB | kC | kD | kE | kF;
        case '1': return kB | kC;
        case '2': return kA | kB | kG | kE | kD;
        case '3': return kA | kB | kG | kC | kD;
        case '4': return kF | kG | kB | kC;
        case '5': return kA | kF | kG | kC | kD;
        case '6': return kA | kF | kG | kE | kC | kD;
        case '7': return kA | kB | kC;
        case '8': return kA | kB | kC | kD | kE | kF | kG;
        case '9': return kA | kB | kC | kD | kF | kG;
        case '-': return kG;
        default: return 0;
    }
}

void bar(SDL_Renderer* renderer, int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    const SDL_Rect rect{x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

}  // namespace

void drawSevenSegmentGlyph(SDL_Renderer* renderer, int x, int y, int width, int height,
                           int thickness, char glyph) {
    if (renderer == nullptr || width <= 0 || height <= 0 || thickness <= 0) return;

    if (glyph == ':') {
        const int dot = thickness;
        bar(renderer, x + (width - dot) / 2, y + height / 3 - dot / 2, dot, dot);
        bar(renderer, x + (width - dot) / 2, y + 2 * height / 3 - dot / 2, dot, dot);
        return;
    }
    if (glyph == ' ') return;

    const uint8_t segments = segmentsFor(glyph);
    const int t = thickness;
    const int half = height / 2;

    if (segments & kA) bar(renderer, x + t, y, width - 2 * t, t);
    if (segments & kD) bar(renderer, x + t, y + height - t, width - 2 * t, t);
    if (segments & kG) bar(renderer, x + t, y + half - t / 2, width - 2 * t, t);
    if (segments & kF) bar(renderer, x, y + t, t, half - t - t / 2);
    if (segments & kB) bar(renderer, x + width - t, y + t, t, half - t - t / 2);
    if (segments & kE) bar(renderer, x, y + half + t / 2, t, half - t);
    if (segments & kC) bar(renderer, x + width - t, y + half + t / 2, t, half - t);
}

int sevenSegmentTextWidth(int digitWidth, int thickness, std::string_view text) {
    if (text.empty()) return 0;
    const int advance = digitWidth + thickness;
    return static_cast<int>(text.size()) * advance - thickness;
}

int drawSevenSegmentText(SDL_Renderer* renderer, int x, int y, int digitWidth, int digitHeight,
                         int thickness, std::string_view text) {
    int cursor = x;
    for (char glyph : text) {
        drawSevenSegmentGlyph(renderer, cursor, y, digitWidth, digitHeight, thickness, glyph);
        cursor += digitWidth + thickness;
    }
    return sevenSegmentTextWidth(digitWidth, thickness, text);
}

}  // namespace retrodisc
