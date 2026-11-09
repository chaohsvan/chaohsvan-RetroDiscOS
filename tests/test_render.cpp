#include <SDL2/SDL.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "display/font5x7.h"
#include "display/text_renderer.h"
#include "display/theme.h"
#include "test_util.h"
#include "ui/main_screen.h"

using retrodisc::UiModel;

namespace {

// Headless SDL: the dummy video driver lets the whole UI run in CI.
struct SdlFixture {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    explicit SdlFixture(int width = 800, int height = 480) {
        setenv("SDL_VIDEODRIVER", "dummy", 1);
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("retrodisc-test", SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_HIDDEN);
        if (window != nullptr) {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        }
    }

    ~SdlFixture() {
        if (renderer != nullptr) SDL_DestroyRenderer(renderer);
        if (window != nullptr) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    SdlFixture(const SdlFixture&) = delete;
    SdlFixture& operator=(const SdlFixture&) = delete;

    bool ok() const {
        return renderer != nullptr;
    }
};

int countLitPixels(SDL_Renderer* renderer, int width, int height) {
    std::vector<uint32_t> pixels(static_cast<std::size_t>(width) * height, 0);
    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, pixels.data(),
                             width * 4) != 0) {
        return -1;
    }
    int lit = 0;
    for (uint32_t pixel : pixels) {
        if ((pixel & 0x00FFFFFFu) != 0) ++lit;
    }
    return lit;
}

void clearToBlack(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

}  // namespace

RD_TEST(headless_renderer_is_available) {
    SdlFixture fixture(320, 200);
    RD_CHECK(fixture.ok());
}

RD_TEST(main_screen_draws_something) {
    SdlFixture fixture(800, 480);
    RD_CHECK(fixture.ok());
    if (!fixture.ok()) return;

    UiModel model;
    model.trackNumber = 1;
    model.trackCount = 12;
    model.trackTitle = "MOCK TRACK 1";
    model.elapsedMs = 83000;
    model.totalMs = 225000;
    model.state = "PLAYING";
    model.volumePercent = 80;
    model.discStatus = "DISC READY";
    model.audioDevice = "null";

    retrodisc::renderMainScreen(fixture.renderer, model, retrodisc::defaultTheme());
    const int lit = countLitPixels(fixture.renderer, 800, 480);
    RD_CHECK(lit > 2000);
}

RD_TEST(main_screen_handles_empty_model) {
    SdlFixture fixture(640, 400);
    RD_CHECK(fixture.ok());
    if (!fixture.ok()) return;

    UiModel empty;
    retrodisc::renderMainScreen(fixture.renderer, empty, retrodisc::defaultTheme());
    const int lit = countLitPixels(fixture.renderer, 640, 400);
    RD_CHECK(lit >= 0);  // must not crash, may draw very little
}

RD_TEST(main_screen_ignores_null_renderer) {
    UiModel model;
    retrodisc::renderMainScreen(nullptr, model, retrodisc::defaultTheme());  // must not crash
    RD_CHECK(true);
}

RD_TEST(text_width_scales_linearly) {
    RD_CHECK_EQ(retrodisc::textWidth("", 3), 0);
    RD_CHECK_EQ(retrodisc::textWidth("A", 3), retrodisc::kGlyphWidth * 3);
    RD_CHECK_EQ(retrodisc::textWidth("AB", 3), (2 * retrodisc::kGlyphAdvance - 1) * 3);
    RD_CHECK_EQ(retrodisc::textWidth("AB", 6), retrodisc::textWidth("AB", 3) * 2);
}

RD_TEST(fit_text_truncates_to_width) {
    const std::string longText = "0123456789012345678901234567890123456789";
    const std::string_view fitted = retrodisc::fitText(longText, 2, 100);
    RD_CHECK(retrodisc::textWidth(fitted, 2) <= 100);
    RD_CHECK(!fitted.empty());
    RD_CHECK(retrodisc::fitText("ABC", 2, 1000) == std::string_view("ABC"));
}

RD_TEST(font_covers_expected_characters) {
    RD_CHECK(retrodisc::glyphRows('A') != nullptr);
    RD_CHECK(retrodisc::glyphRows('Z') != nullptr);
    RD_CHECK(retrodisc::glyphRows('0') != nullptr);
    RD_CHECK(retrodisc::glyphRows(':') != nullptr);
    RD_CHECK(retrodisc::glyphRows('@') == nullptr);
}

RD_TEST(glyph_density_differs_between_digits) {
    SdlFixture fixture(120, 60);
    RD_CHECK(fixture.ok());
    if (!fixture.ok()) return;

    clearToBlack(fixture.renderer);
    SDL_SetRenderDrawColor(fixture.renderer, 255, 255, 255, 255);
    retrodisc::drawText(fixture.renderer, 10, 10, 4, "1");
    const int one = countLitPixels(fixture.renderer, 120, 60);

    clearToBlack(fixture.renderer);
    SDL_SetRenderDrawColor(fixture.renderer, 255, 255, 255, 255);
    retrodisc::drawText(fixture.renderer, 10, 10, 4, "8");
    const int eight = countLitPixels(fixture.renderer, 120, 60);

    RD_CHECK(one > 0);
    RD_CHECK(eight > one);
}

RD_TEST(track_counter_formatting) {
    RD_CHECK_EQ(retrodisc::formatTrackCounter(1, 12), std::string("01/12"));
    RD_CHECK_EQ(retrodisc::formatTrackCounter(9, 9), std::string("09/09"));
    RD_CHECK_EQ(retrodisc::formatTrackCounter(0, 0), std::string("--/--"));
}

RD_TEST(theme_colours_reach_the_screen) {
    SdlFixture fixture(400, 240);
    RD_CHECK(fixture.ok());
    if (!fixture.ok()) return;

    UiModel model;
    model.trackNumber = 1;
    model.trackCount = 12;
    model.trackTitle = "TEST";
    model.totalMs = 1000;
    model.state = "PLAYING";

    retrodisc::renderMainScreen(fixture.renderer, model, *retrodisc::themeById("amber"));

    std::vector<uint32_t> pixels(400u * 240u, 0);
    if (SDL_RenderReadPixels(fixture.renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, pixels.data(),
                             400 * 4) != 0) {
        RD_CHECK(false);
        return;
    }

    uint32_t brightest = 0;
    int bestSum = -1;
    for (uint32_t pixel : pixels) {
        const int sum = static_cast<int>((pixel >> 16) & 0xFF) +
                        static_cast<int>((pixel >> 8) & 0xFF) + static_cast<int>(pixel & 0xFF);
        if (sum > bestSum) {
            bestSum = sum;
            brightest = pixel;
        }
    }
    const int r = static_cast<int>((brightest >> 16) & 0xFF);
    const int g = static_cast<int>((brightest >> 8) & 0xFF);
    const int b = static_cast<int>(brightest & 0xFF);
    RD_CHECK(r > g);  // amber is red-dominant
    RD_CHECK(g > b);
}

RD_TEST(off_theme_renders_a_blank_screen) {
    SdlFixture fixture(200, 120);
    RD_CHECK(fixture.ok());
    if (!fixture.ok()) return;

    UiModel model;
    model.state = "PLAYING";
    retrodisc::renderMainScreen(fixture.renderer, model, *retrodisc::themeById("off"));
    RD_CHECK_EQ(countLitPixels(fixture.renderer, 200, 120), 0);
}

RD_TEST_MAIN()
