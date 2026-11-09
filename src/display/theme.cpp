#include "display/theme.h"

namespace retrodisc {
namespace {

const std::vector<Theme>& registry() {
    static const std::vector<Theme> themes = {
        {"digital-hifi",
         "DIGITAL HI-FI",
         {6, 10, 8, 255},
         {90, 255, 150, 255},
         {30, 110, 60, 255},
         {210, 255, 225, 255},
         false},
        {"classic-vfd",
         "CLASSIC VFD",
         {4, 8, 10, 255},
         {70, 240, 220, 255},
         {20, 96, 88, 255},
         {190, 255, 245, 255},
         false},
        {"amber",
         "AMBER",
         {10, 6, 3, 255},
         {255, 179, 71, 255},
         {122, 74, 18, 255},
         {255, 226, 170, 255},
         false},
        {"vintage-analog",
         "VINTAGE ANALOG",
         {11, 8, 6, 255},
         {232, 217, 176, 255},
         {106, 90, 58, 255},
         {255, 246, 220, 255},
         false},
        {"off",
         "DISPLAY OFF",
         {0, 0, 0, 255},
         {0, 0, 0, 255},
         {0, 0, 0, 255},
         {0, 0, 0, 255},
         true},
    };
    return themes;
}

}  // namespace

const std::vector<Theme>& allThemes() {
    return registry();
}

const Theme& defaultTheme() {
    return registry().front();
}

const Theme* themeById(std::string_view id) {
    for (const auto& theme : registry()) {
        if (theme.id == id) return &theme;
    }
    return nullptr;
}

const Theme& themeOrFallback(std::string_view id) {
    const Theme* theme = themeById(id);
    return theme != nullptr ? *theme : defaultTheme();
}

std::string nextThemeId(std::string_view id) {
    const auto& themes = registry();
    for (std::size_t i = 0; i < themes.size(); ++i) {
        if (themes[i].id == id) {
            return themes[(i + 1) % themes.size()].id;
        }
    }
    return themes.front().id;
}

}  // namespace retrodisc
