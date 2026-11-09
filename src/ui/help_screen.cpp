#include "ui/help_screen.h"

#include <SDL2/SDL.h>

#include <algorithm>

#include "display/text_renderer.h"
#include "display/theme.h"

namespace retrodisc {
namespace {

constexpr int kPageSize = 11;  // lines that fit at 800x480 with scale 3
constexpr int kMarginDivisor = 24;

std::vector<HelpSection> buildSections() {
    return {
        {"QUICK START",
         {"1. POWER ON: THE PLAYER STARTS FULL SCREEN.", "2. PUSH A CD IN BY HAND.",
          "3. PLAYBACK STARTS AUTOMATICALLY.", "4. SPACE PAUSES AND RESUMES."}},
        {"KEYS",
         {"SPACE PLAY/PAUSE   LEFT RIGHT PREV/NEXT", "UP DOWN VOLUME          F10 MUTE",
          "F1 MODE  F2 RANDOM  F3 REPEAT", "F5 THEME   F6 F7 BRIGHTNESS   F8 DISPLAY",
          "F11 F12 SEEK   E EJECT   H HELP", "S SERVICE MODE          Q QUIT"}},
        {"CD PLAYBACK",
         {"AUDIO IS READ DIRECTLY FROM THE DISC.", "TRACK, TITLE AND TIME ARE SHOWN.",
          "NO CD-TEXT SHOWS AS 'TRACK 01'.", "SEEK MOVES IN WHOLE SECTORS."}},
        {"PLAY MODES",
         {"NORMAL   TRACKS IN ORDER, THEN STOP.", "REPEAT 1 LOOP THE CURRENT TRACK.",
          "REPEAT ALL LOOP THE WHOLE DISC.", "RANDOM   PLAY IN RANDOM ORDER."}},
        {"THEMES",
         {"F5 CYCLES: DIGITAL HI-FI, CLASSIC VFD,", "AMBER, VINTAGE ANALOG, DISPLAY OFF.",
          "THE CHOICE IS SAVED ON EXIT."}},
        {"BRIGHTNESS",
         {"F6 F7 STEP 100 75 50 25 0 PERCENT.", "UI LIGHT AND BACKLIGHT ARE SEPARATE.",
          "AT 0 PERCENT ANY KEY WAKES IT."}},
        {"DISC DRIVE",
         {"E OPENS THE TRAY. SOFTWARE CANNOT CLOSE IT,", "SO PUSH THE TRAY IN BY HAND.",
          "REMOVING A DISC STOPS PLAYBACK SAFELY."}},
        {"AUDIO OUTPUT",
         {"DEVICE COMES FROM config/player.conf.", "VOLUME IS SOFTWARE VOLUME, NOT BIT-PERFECT.",
          "A LOST DEVICE STOPS PLAYBACK, NO CRASH."}},
        {"SYSTEM",
         {"S OPENS SERVICE MODE WITH HARDWARE STATUS.",
          "CONFIG LIVES IN config/ NEXT TO THE BINARY.", "STATE IS SAVED UNDER ~/.local/state."}},
        {"TROUBLESHOOTING",
         {"NO DISC:      CHECK THE TRAY IS CLOSED.", "NO SOUND:     CHECK audio_device IN CONFIG.",
          "READ ERRORS:  CLEAN THE DISC AND RETRY.", "STUCK:        Q TO QUIT, THEN RESTART."}},
    };
}

}  // namespace

HelpScreen::HelpScreen() : sections_(buildSections()) {}

int HelpScreen::pageSize() {
    return kPageSize;
}

void HelpScreen::show() {
    visible_ = true;
    scroll_ = 0;
}

void HelpScreen::hide() {
    visible_ = false;
}

void HelpScreen::toggle() {
    if (visible_) {
        hide();
    } else {
        show();
    }
}

int HelpScreen::totalLines() const {
    int total = 0;
    for (const auto& section : sections_) {
        total += 1 + static_cast<int>(section.lines.size());  // title + body
    }
    return total;
}

void HelpScreen::scrollBy(int delta) {
    const int maxScroll = std::max(0, totalLines() - kPageSize);
    scroll_ = std::clamp(scroll_ + delta, 0, maxScroll);
}

void HelpScreen::scrollToTop() {
    scroll_ = 0;
}

void HelpScreen::scrollToBottom() {
    scroll_ = std::max(0, totalLines() - kPageSize);
}

void renderHelpScreen(SDL_Renderer* renderer, const HelpScreen& help, const Theme& theme) {
    if (renderer == nullptr) return;

    int width = 0;
    int height = 0;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0) return;
    if (width <= 0 || height <= 0) return;

    SDL_SetRenderDrawColor(renderer, theme.background.r, theme.background.g, theme.background.b,
                           theme.background.a);
    SDL_RenderClear(renderer);

    const int margin = std::max(8, width / kMarginDivisor);

    int longest = 1;
    for (const auto& section : help.sections()) {
        longest = std::max(longest, static_cast<int>(section.title.size()));
        for (const auto& line : section.lines) {
            longest = std::max(longest, static_cast<int>(line.size()));
        }
    }
    const int byWidth = (width - 2 * margin) / (longest * kGlyphAdvance);
    const int scale = std::max(2, std::min({height / 160, 4, byWidth}));
    const int lineHeight = textHeight(scale) + scale * 3;

    // Header
    SDL_SetRenderDrawColor(renderer, theme.foreground.r, theme.foreground.g, theme.foreground.b,
                           theme.foreground.a);
    drawText(renderer, margin, margin, scale + 1, "HELP");
    drawText(renderer, width - margin - textWidth("H CLOSE", scale), margin, scale, "H CLOSE");

    // Flattened lines: (isTitle, text)
    std::vector<std::pair<bool, const std::string*>> lines;
    for (const auto& section : help.sections()) {
        lines.emplace_back(true, &section.title);
        for (const auto& line : section.lines) lines.emplace_back(false, &line);
    }

    int y = margin + textHeight(scale + 1) + margin / 2;
    const int footerY = height - margin - textHeight(scale);
    const int bottom = footerY - lineHeight;
    for (int i = help.scroll(); i < static_cast<int>(lines.size()) && y <= bottom; ++i) {
        const bool isTitle = lines[static_cast<std::size_t>(i)].first;
        const std::string& text = *lines[static_cast<std::size_t>(i)].second;
        const SDL_Color color = isTitle ? theme.accent : theme.foreground;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        drawText(renderer, margin + (isTitle ? 0 : scale * 3), y, scale,
                 fitText(text, scale, width - 2 * margin));
        y += lineHeight;
    }

    // Footer: scroll position
    const int maxScroll = std::max(0, help.totalLines() - HelpScreen::pageSize());
    const std::string footer =
        std::to_string(help.scroll()) + "/" + std::to_string(maxScroll) + "  UP/DOWN SCROLL";
    SDL_SetRenderDrawColor(renderer, theme.dim.r, theme.dim.g, theme.dim.b, theme.dim.a);
    drawText(renderer, margin, footerY, scale, fitText(footer, scale, width - 2 * margin));
}

}  // namespace retrodisc
