#include "ui/main_screen.h"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "cd/cd_interface.h"
#include "display/seven_segment.h"
#include "display/text_renderer.h"
#include "display/vu_meter.h"

namespace retrodisc {
namespace {

constexpr int kMarginDivisor = 24;

int clampInt(int value, int low, int high) {
    return std::max(low, std::min(value, high));
}

void setColor(SDL_Renderer* renderer, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
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

void renderMainScreen(SDL_Renderer* renderer, const UiModel& model, const Theme& theme) {
    if (renderer == nullptr) return;

    int width = 0;
    int height = 0;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0) return;
    if (width <= 0 || height <= 0) return;

    setColor(renderer, theme.background);
    SDL_RenderClear(renderer);
    if (theme.displayOff) return;  // T10: blank screen, playback keeps running

    const int margin = std::max(8, width / kMarginDivisor);
    const int scaleSmall = clampInt(height / 120, 2, 6);
    // Reserve the right third for the total time so the big clock never runs off screen.
    const int bigBudget = (width * 62) / 100 - margin;
    const int scaleBig = clampInt(std::min(height / 16, bigBudget / (5 * kGlyphAdvance)), 4, 28);
    const int statusScale = std::max(2, scaleSmall - 1);
    const int lineHeight = textHeight(statusScale) + margin / 4;

    // --- Layout ------------------------------------------------------------
    const int barHeight = std::max(6, height / 60);
    const int barY = height - margin - barHeight;
    const int statusY = barY - textHeight(statusScale) - margin / 2;
    const int deviceY = statusY - lineHeight;
    const int timeY = margin + textHeight(scaleSmall) + margin / 2;
    const int timeBottom = timeY + textHeight(scaleBig);

    // --- Row 1: track counter and title -----------------------------------
    const std::string counter = formatTrackCounter(model.trackNumber, model.trackCount);
    setColor(renderer, theme.foreground);
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

    int clockWidth = textWidth(elapsed, scaleBig);
    if (theme.clockStyle == ClockStyle::SevenSegment) {
        // Segmented VFD clock: height driven by the width budget, not the font.
        int digitHeight = std::min(height / 5, (bigBudget * 10) / 32);
        if (digitHeight < 12) digitHeight = 12;
        int thickness = std::max(3, digitHeight / 9);
        int digitWidth = (digitHeight * 11) / 20;
        while (digitWidth > 1 &&
               sevenSegmentTextWidth(digitWidth, thickness, elapsed) > bigBudget) {
            --digitWidth;
            if (digitWidth < 4) break;
        }
        setColor(renderer, theme.foreground);
        clockWidth = drawSevenSegmentText(renderer, margin, timeY, digitWidth, digitHeight,
                                          thickness, elapsed);
    } else {
        drawText(renderer, margin, timeY, scaleBig, elapsed);
    }

    setColor(renderer, theme.dim);
    const int totalX = margin + clockWidth + kGlyphAdvance * scaleBig;
    const int totalChars = 7;  // " /03:45" plus slack
    const int totalRoom = std::max(0, width - margin - totalX);
    const int totalScale =
        std::max(2, std::min(scaleBig / 2, totalRoom / (totalChars * kGlyphAdvance)));
    const int totalY = timeY + textHeight(scaleBig) - textHeight(totalScale);
    drawText(renderer, totalX, totalY, totalScale, "/" + total);

    // --- Analog VU meter (vintage theme) -----------------------------------
    if (theme.vuMeter) {
        const int top = timeBottom + margin / 2;
        const int bottom = deviceY - margin / 2;
        const int meterHeight = bottom - top;
        if (meterHeight > textHeight(statusScale) * 2) {
            float leftLevel = 0.0f;
            float rightLevel = 0.0f;
            const int half = SpectrumAnalyzer::kBands / 2;
            for (int band = 0; band < half; ++band) {
                leftLevel += model.spectrum[static_cast<std::size_t>(band)];
                rightLevel +=
                    model.spectrum[static_cast<std::size_t>(band + half)];
            }
            leftLevel /= static_cast<float>(half);
            rightLevel /= static_cast<float>(half);
            drawVuMeter(renderer, margin, top, width - 2 * margin, meterHeight, leftLevel,
                        rightLevel, theme);
        }
    } else if (model.showSpectrum) {
        const int top = timeBottom + margin / 2;
        const int bottom = deviceY - margin / 2;
        const int available = bottom - top;
        if (available > textHeight(statusScale)) {
            const int bandWidth = (width - 2 * margin) / SpectrumAnalyzer::kBands;
            for (int band = 0; band < SpectrumAnalyzer::kBands; ++band) {
                const float level =
                    std::clamp(model.spectrum[static_cast<std::size_t>(band)], 0.0f, 1.0f);
                const int barHeightPixels = std::max(1, static_cast<int>(level * available));
                const SDL_Rect bar{margin + band * bandWidth + 1, bottom - barHeightPixels,
                                   std::max(1, bandWidth - 3), barHeightPixels};
                setColor(renderer, band % 3 == 0 ? theme.accent : theme.foreground);
                SDL_RenderFillRect(renderer, &bar);
            }
        }
    }

    // --- Progress bar ------------------------------------------------------
    setColor(renderer, theme.dim);
    const SDL_Rect barBackground{margin, barY, width - 2 * margin, barHeight};
    SDL_RenderFillRect(renderer, &barBackground);

    if (model.totalMs > 0) {
        const double ratio =
            std::min(1.0, std::max(0.0, static_cast<double>(model.elapsedMs) / model.totalMs));
        setColor(renderer, theme.accent);
        const SDL_Rect filled{margin, barY, static_cast<int>((width - 2 * margin) * ratio),
                              barHeight};
        SDL_RenderFillRect(renderer, &filled);
    }

    // --- Rows 3/4: state, volume, mode, disc and device --------------------
    std::string status = model.state;
    if (!model.playMode.empty()) {
        status += "  ";
        status += model.playMode;
    }
    status += "  VOL ";
    status += std::to_string(clampInt(model.volumePercent, 0, 100));
    status += "%";
    setColor(renderer, theme.foreground);
    drawText(renderer, margin, statusY, statusScale, status);

    const std::string deviceLine = model.discStatus + "  " + model.audioDevice;
    setColor(renderer, theme.dim);
    drawText(renderer, margin, deviceY, statusScale,
             fitText(deviceLine, statusScale, width - 2 * margin));
}

}  // namespace retrodisc
