#pragma once

#include <array>
#include <string>

#include "core/spectrum.h"
#include "display/theme.h"

struct SDL_Renderer;

namespace retrodisc {

// Everything the main screen needs to draw. The UI owns no state of its own:
// it receives a snapshot and renders it (AGENTS.md 5.2).
struct UiModel {
    int trackNumber = 0;
    int trackCount = 0;
    std::string trackTitle;
    int elapsedMs = 0;
    int totalMs = 0;
    std::string state = "NO_DISC";
    int volumePercent = 80;
    std::string discStatus = "NO DISC";
    std::string audioDevice = "null";
    std::string playMode = "NORMAL";
    bool showSpectrum = false;
    int animationMs = 0;        // monotonic clock, drives the blinking colon
    double discAngleDeg = 0.0;  // frozen while paused
    std::array<float, SpectrumAnalyzer::kBands> spectrum{};
};

// "01/12", zero padded like a real CD player.
std::string formatTrackCounter(int trackNumber, int trackCount);

// Clears the renderer, draws the player screen with the given theme, and does
// NOT call SDL_RenderPresent. A displayOff theme renders an empty black screen.
void renderMainScreen(SDL_Renderer* renderer, const UiModel& model, const Theme& theme);

}  // namespace retrodisc
