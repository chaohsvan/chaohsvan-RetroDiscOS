#include "display/text_renderer.h"

#include <SDL2/SDL.h>

#include "display/font5x7.h"

namespace retrodisc {
namespace {

char upcase(char c) {
    if (c >= 'a' && c <= 'z') return static_cast<char>(c - 'a' + 'A');
    return c;
}

}  // namespace

int textWidth(std::string_view text, int scale) {
    if (text.empty() || scale <= 0) return 0;
    // The advance includes the trailing gap; the last glyph does not need it.
    return static_cast<int>(text.size()) * kGlyphAdvance * scale - scale;
}

int textHeight(int scale) {
    return kGlyphHeight * scale;
}

void drawText(SDL_Renderer* renderer, int x, int y, int scale, std::string_view text) {
    if (renderer == nullptr || scale <= 0) return;

    int cursor = x;
    for (char raw : text) {
        const char c = upcase(raw);
        const uint8_t* rows = glyphRows(c);
        if (rows != nullptr) {
            for (int row = 0; row < kGlyphHeight; ++row) {
                const uint8_t bits = rows[row];
                if (bits == 0) continue;
                for (int col = 0; col < kGlyphWidth; ++col) {
                    if ((bits & (1u << (kGlyphWidth - 1 - col))) == 0) continue;
                    const SDL_Rect pixel{cursor + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        cursor += kGlyphAdvance * scale;
    }
}

std::string_view fitText(std::string_view text, int scale, int maxWidth) {
    if (maxWidth <= 0 || scale <= 0) return std::string_view{};
    std::size_t count = text.size();
    while (count > 0 && textWidth(text.substr(0, count), scale) > maxWidth) --count;
    return text.substr(0, count);
}

}  // namespace retrodisc
