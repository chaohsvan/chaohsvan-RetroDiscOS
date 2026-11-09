#include "ui/main_screen.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "cd/cd_interface.h"
#include "display/text_renderer.h"

namespace retrodisc {
namespace {

constexpr int kMarginDivisor = 24;
constexpr SDL_Color kBackground{6, 10, 8, 255};
constexpr SDL_Color kForeground{90, 255, 150, 255};
constexpr SDL_Color kDim{30, 110, 60, 255};

int clampInt(int value, int low, int high) {
    return std::max(low, std::min(value, high));
}

}  // namespace

std::string formatTrackCounter(int trackNumber, int trackCount) {
    char buffer[24];
    if (trackCount <= 0) {
        std::snprintf(buffer, sizeof(buffer), "--/--");
    } else {
        std::snprintf(buffer, sizeof(buffer), "%02d/%02d", trackNumber, trackCount);
    }
    return std::string(buffer);
}

void renderMainScreen(SDL_Renderer* renderer, const UiModel& model) {
    if (renderer == nullptr) return;

    int width = 0;
    int height = 0;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0) return;
    if (width <= 0 || height <= 0) return;

    SDL_SetRenderDrawColor(renderer, kBackground.r, kBackground.g, kBackground.b, kBackground.a);
    SDL_RenderClear(renderer);

    const int margin = std::max(8, width / kMarginDivisor);
    const int scaleSmall = clampInt(height / 120, 2, 6);
    // Reserve the right third for the total time so the big clock never runs off screen.
    const int bigBudget = (width * 62) / 100 - margin;
    const int scaleBig = clampInt(std::min(height / 16, bigBudget / (5 * kGlyphAdvance)), 4, 28);

    // --- Row 1: track counter and title -----------------------------------
    const std::string counter = formatTrackCounter(model.trackNumber, model.trackCount);
    SDL_SetRenderDrawColor(renderer, kForeground.r, kForeground.g, kForeground.b, kForeground.a);
    drawText(renderer, margin, margin, scaleSmall, counter);

    const int titleX = margin + (static_cast<int>(counter.size()) + 2) * kGlyphAdvance * scaleSmall;
    if (titleX < width - margin) {
        const std::string& title =
            model.trackTitle.empty() ? std::string("NO TITLE") : model.trackTitle;
        drawText(renderer, titleX, margin, scaleSmall,
                 fitText(title, scaleSmall, width - titleX - margin));
    }

    // --- Row 2: elapsed / total -------------------------------------------
    const std::string elapsed = formatTime(model.elapsedMs);
    const std::string total = formatTime(model.totalMs);
    const int timeY = margin + textHeight(scaleSmall) + margin / 2;
    drawText(renderer, margin, timeY, scaleBig, elapsed);

    SDL_SetRenderDrawColor(renderer, kDim.r, kDim.g, kDim.b, kDim.a);
    const int totalX = margin + textWidth(elapsed, scaleBig) + kGlyphAdvance * scaleBig;
    const int totalChars = 7;  // " /03:45" plus slack
    const int totalRoom = std::max(0, width - margin - totalX);
    const int totalScale =
        std::max(2, std::min(scaleBig / 2, totalRoom / (totalChars * kGlyphAdvance)));
    const int totalY = timeY + textHeight(scaleBig) - textHeight(totalScale);
    drawText(renderer, totalX, totalY, totalScale, "/" + total);

    // --- Progress bar ------------------------------------------------------
    const int barHeight = std::max(6, height / 60);
    const int barY = height - margin - barHeight;
    SDL_SetRenderDrawColor(renderer, kDim.r, kDim.g, kDim.b, kDim.a);
    const SDL_Rect barBackground{margin, barY, width - 2 * margin, barHeight};
    SDL_RenderFillRect(renderer, &barBackground);

    if (model.totalMs > 0) {
        const double ratio =
            std::min(1.0, std::max(0.0, static_cast<double>(model.elapsedMs) / model.totalMs));
        SDL_SetRenderDrawColor(renderer, kForeground.r, kForeground.g, kForeground.b,
                               kForeground.a);
        const SDL_Rect filled{margin, barY, static_cast<int>((width - 2 * margin) * ratio),
                              barHeight};
        SDL_RenderFillRect(renderer, &filled);
    }

    // --- Row 3: state and volume ------------------------------------------
    const int statusScale = std::max(2, scaleSmall - 1);
    const int statusY = barY - textHeight(statusScale) - margin / 2;
    std::string status = model.state;
    status += "  VOL ";
    status += std::to_string(clampInt(model.volumePercent, 0, 100));
    status += "%";
    SDL_SetRenderDrawColor(renderer, kForeground.r, kForeground.g, kForeground.b, kForeground.a);
    drawText(renderer, margin, statusY, statusScale, status);

    const int heightOffset = textHeight(statusScale) + margin / 4;
    const std::string deviceLine = model.discStatus + "  " + model.audioDevice;
    SDL_SetRenderDrawColor(renderer, kDim.r, kDim.g, kDim.b, kDim.a);
    drawText(renderer, margin, statusY - heightOffset, statusScale,
             fitText(deviceLine, statusScale, width - 2 * margin));
}

}  // namespace retrodisc
