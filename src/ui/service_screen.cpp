#include "ui/service_screen.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <string>
#include <utility>

#include "display/text_renderer.h"
#include "display/theme.h"

namespace retrodisc {
namespace {

constexpr int kPageSize = 10;
constexpr int kMarginDivisor = 24;
constexpr int kLabelColumn = 12;  // characters reserved for the label

}  // namespace

ServiceScreen::ServiceScreen(Diagnostics diagnostics)
    : diagnostics_(std::move(diagnostics)), items_(diagnostics_.collect()) {}

int ServiceScreen::pageSize() {
    return kPageSize;
}

void ServiceScreen::show() {
    visible_ = true;
    scroll_ = 0;
    refresh();
}

void ServiceScreen::hide() {
    visible_ = false;
}

void ServiceScreen::toggle() {
    if (visible_) {
        hide();
    } else {
        show();
    }
}

void ServiceScreen::refresh() {
    items_ = diagnostics_.collect();
}

void ServiceScreen::scrollBy(int delta) {
    const int maxScroll = std::max(0, static_cast<int>(items_.size()) - kPageSize);
    scroll_ = std::clamp(scroll_ + delta, 0, maxScroll);
}

void renderServiceScreen(SDL_Renderer* renderer, const ServiceScreen& screen, const Theme& theme) {
    if (renderer == nullptr) return;

    int width = 0;
    int height = 0;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0) return;
    if (width <= 0 || height <= 0) return;

    SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b,
                           theme.background.a);
    SDL_RenderClear(renderer);

    const int margin = std::max(8, width / kMarginDivisor);
    const int scale = std::max(2, std::min(height / 150, 4));
    const int lineHeight = textHeight(scale) + scale * 4;

    SDL_SetRenderDrawColor(renderer, theme.foreground.r, theme.foreground.g, theme.foreground.b,
                           theme.foreground.a);
    drawText(renderer, margin, margin, scale + 1, "SERVICE MODE");
    drawText(renderer, width - margin - textWidth("S CLOSE", scale), margin + scale, scale,
             "S CLOSE");

    int y = margin + textHeight(scale + 1) + margin / 2;
    const int footerY = height - margin - textHeight(scale);
    const int bottom = footerY - lineHeight;

    const auto& items = screen.items();
    for (int i = screen.scroll(); i < static_cast<int>(items.size()) && y <= bottom; ++i) {
        const DiagnosticItem& item = items[static_cast<std::size_t>(i)];

        SDL_SetRenderDrawColor(renderer, theme.accent.r, theme.accent.g, theme.accent.b,
                               theme.accent.a);
        drawText(renderer, margin, y, scale, item.label);

        const SDL_Color color = item.available ? theme.foreground : theme.dim;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        drawText(renderer, margin + kLabelColumn * kGlyphAdvance * scale, y, scale,
                 fitText(item.value, scale,
                         width - margin - (margin + kLabelColumn * kGlyphAdvance * scale)));
        y += lineHeight;
    }

    const int maxScroll = std::max(0, static_cast<int>(items.size()) - kPageSize);
    const std::string footer =
        std::to_string(screen.scroll()) + "/" + std::to_string(maxScroll) + "  UP/DOWN  S CLOSE";
    SDL_SetRenderDrawColor(renderer, theme.dim.r, theme.dim.g, theme.dim.b, theme.dim.a);
    drawText(renderer, margin, footerY, scale, fitText(footer, scale, width - 2 * margin));
}

}  // namespace retrodisc
