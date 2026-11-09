#pragma once

#include <string>
#include <vector>

struct SDL_Renderer;

namespace retrodisc {

struct Theme;

struct HelpSection {
    std::string title;
    std::vector<std::string> lines;
};

// Offline help (PRD 2.7). Content is compiled in, so it works with no network
// and no external files. Text is ASCII because the UI font is 5x7.
class HelpScreen {
   public:
    HelpScreen();

    const std::vector<HelpSection>& sections() const {
        return sections_;
    }
    std::size_t sectionCount() const {
        return sections_.size();
    }

    bool visible() const {
        return visible_;
    }
    void show();
    void hide();
    void toggle();

    int scroll() const {
        return scroll_;
    }
    void scrollBy(int delta);
    void scrollToTop();
    void scrollToBottom();

    // Number of rendered lines (section titles included).
    int totalLines() const;
    static int pageSize();

   private:
    std::vector<HelpSection> sections_;
    int scroll_ = 0;
    bool visible_ = false;
};

// Draws the help page for the current scroll offset. Does not call SDL_RenderPresent.
void renderHelpScreen(SDL_Renderer* renderer, const HelpScreen& help, const Theme& theme);

}  // namespace retrodisc
