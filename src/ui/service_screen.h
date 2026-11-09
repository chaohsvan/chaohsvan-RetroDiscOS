#pragma once

#include <vector>

#include "system/diagnostics.h"

struct SDL_Renderer;

namespace retrodisc {

struct Theme;

// SERVICE MODE screen (PRD 2.8). Read-only: it never touches fan control,
// partitions or kernel settings.
class ServiceScreen {
   public:
    explicit ServiceScreen(Diagnostics diagnostics = Diagnostics());

    bool visible() const {
        return visible_;
    }
    void show();
    void hide();
    void toggle();

    void refresh();
    const std::vector<DiagnosticItem>& items() const {
        return items_;
    }

    int scroll() const {
        return scroll_;
    }
    void scrollBy(int delta);
    static int pageSize();

   private:
    Diagnostics diagnostics_;
    std::vector<DiagnosticItem> items_;
    int scroll_ = 0;
    bool visible_ = false;
};

void renderServiceScreen(SDL_Renderer* renderer, const ServiceScreen& screen, const Theme& theme);

}  // namespace retrodisc
