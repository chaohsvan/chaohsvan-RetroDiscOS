#pragma once

#include <string>

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
};

// "01/12", zero padded like a real CD player.
std::string formatTrackCounter(int trackNumber, int trackCount);

// Clears the renderer, draws the player screen, and does NOT call SDL_RenderPresent.
void renderMainScreen(SDL_Renderer* renderer, const UiModel& model);

}  // namespace retrodisc
