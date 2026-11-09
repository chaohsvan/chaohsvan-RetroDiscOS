#include <SDL2/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "audio/mock_audio.h"
#include "cd/mock_cd.h"
#include "core/config.h"
#include "core/event_bus.h"
#include "core/player_core.h"
#include "core/resume.h"
#include "core/state_store.h"
#include "core/version.h"
#include "display/backlight.h"
#include "display/brightness.h"
#include "display/display_power.h"
#include "display/theme_manager.h"
#include "input/key_mapping.h"
#include "ui/help_screen.h"
#include "ui/main_screen.h"
#include "ui/service_screen.h"

namespace {

struct Options {
    bool mock = false;
    bool autoDisc = false;
    bool render = false;
    bool fast = false;
    int seconds = 2;
    std::string configDir = "config";
    std::string theme;
    std::string screenshot;
    int brightnessLevel = -1;  // -1 = leave at 100 %
    bool showHelp = false;
    bool showService = false;
    bool spectrum = false;
};

void printUsage() {
    std::printf(
        "RetroDisc OS %s\n"
        "\n"
        "Usage: retrodisc [options]\n"
        "\n"
        "Options:\n"
        "  --mock            Use mock CD/audio backends (no hardware needed)\n"
        "  --auto-disc       Insert a mock disc at startup\n"
        "  --render          Open the SDL2 player window (works with dummy driver)\n"
        "  --theme ID        Start with this theme (digital-hifi, classic-vfd, amber,\n"
        "                    vintage-analog, off)\n"
        "  --seconds N       How long to play before stopping (default 2)\n"
        "  --config DIR      Config directory (default ./config)\n"
        "  --screenshot FILE Save the first frame as a BMP (requires --render)\n"
        "  --brightness N    UI brightness level 0..4 => 100/75/50/25/0 percent\n"
        "  --show-help       Open the help screen at startup (needs --render)\n"
        "  --show-service    Open SERVICE MODE at startup (needs --render)\n"
        "  --spectrum        Enable the F9 spectrum display at startup\n"
        "  --fast            Do not pace audio to real time\n"
        "  --help            Show this help\n"
        "\n"
        "Keys (while --render is active): SPACE play/pause, LEFT/RIGHT prev/next,\n"
        "UP/DOWN volume, F5 theme, E eject, X stop, Q quit.\n",
        std::string(retrodisc::version()).c_str());
}

bool hasFlag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

const char* valueOf(int argc, char** argv, const char* flag) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return nullptr;
}

// ~/.local/state/retrodisc/state.conf, created on demand.
std::string defaultStatePath() {
    const char* home = std::getenv("HOME");
    const std::string base = std::string(home != nullptr ? home : ".") + "/.local/state/retrodisc";
    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    return base + "/state.conf";
}

retrodisc::UiModel makeModel(const retrodisc::PlayerCore& core, const retrodisc::PlayerConfig& cfg,
                             bool spectrumEnabled) {
    retrodisc::UiModel model;
    model.trackNumber = core.trackNumber();
    model.trackCount = core.trackCount();
    model.trackTitle = core.currentTitle();
    model.elapsedMs = core.elapsedMs();
    model.totalMs = core.trackTotalMs();
    model.state = std::string(retrodisc::toString(core.state()));
    model.volumePercent = static_cast<int>(cfg.volume * 100.0f + 0.5f);
    model.discStatus = core.hasDisc() ? "DISC READY" : "NO DISC";
    model.audioDevice = cfg.audioDevice;
    model.playMode = std::string(retrodisc::toString(core.playMode()));
    model.showSpectrum = spectrumEnabled;
    model.spectrum = core.spectrum().levels();
    return model;
}

bool saveFrame(SDL_Renderer* renderer, const std::string& path) {
    int width = 0;
    int height = 0;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0) return false;
    SDL_Surface* surface =
        SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    if (surface == nullptr) return false;
    const bool ok = SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888,
                                         surface->pixels, surface->pitch) == 0 &&
                    SDL_SaveBMP(surface, path.c_str()) == 0;
    SDL_FreeSurface(surface);
    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    if (hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h")) {
        printUsage();
        return 0;
    }

    Options options;
    options.mock = hasFlag(argc, argv, "--mock");
    options.autoDisc = hasFlag(argc, argv, "--auto-disc");
    options.render = hasFlag(argc, argv, "--render");
    options.fast = hasFlag(argc, argv, "--fast");
    if (const char* seconds = valueOf(argc, argv, "--seconds")) {
        options.seconds = std::atoi(seconds);
        if (options.seconds <= 0) options.seconds = 1;
    }
    if (const char* dir = valueOf(argc, argv, "--config")) options.configDir = dir;
    if (const char* theme = valueOf(argc, argv, "--theme")) options.theme = theme;
    if (const char* shot = valueOf(argc, argv, "--screenshot")) options.screenshot = shot;
    options.showHelp = hasFlag(argc, argv, "--show-help");
    options.showService = hasFlag(argc, argv, "--show-service");
    options.spectrum = hasFlag(argc, argv, "--spectrum");
    if (const char* level = valueOf(argc, argv, "--brightness"))
        options.brightnessLevel = std::atoi(level);

    std::printf("RetroDisc OS %s\n", std::string(retrodisc::version()).c_str());

    if (!options.mock) {
        std::fprintf(stderr, "[error] real backends are not implemented yet (milestone M4)\n");
        std::fprintf(stderr, "[error] run with --mock to exercise the player core\n");
        return 2;
    }

    const retrodisc::ConfigResult config = retrodisc::loadConfig(options.configDir);
    for (const auto& warning : config.warnings) {
        std::printf("[config] %s\n", warning.c_str());
    }
    if (!config.ok) {
        std::fprintf(stderr, "[error] invalid configuration\n");
        return 1;
    }
    retrodisc::PlayerConfig cfg = config.config;

    retrodisc::StateStore state(defaultStatePath());
    state.load();
    retrodisc::ThemeManager themes(
        options.theme.empty() ? retrodisc::ThemeManager::initialIdFromStore(state, cfg.theme)
                              : options.theme,
        &state);
    if (!options.theme.empty() && themes.currentId() != options.theme) {
        std::printf("[theme] unknown theme '%s', using '%s'\n", options.theme.c_str(),
                    themes.currentId().c_str());
    }
    std::printf("[theme] %s (%s)\n", themes.current().name.c_str(), themes.currentId().c_str());

    const retrodisc::KeyMap keys(cfg.keys);
    retrodisc::SysfsBacklight backlightDevice;
    retrodisc::BrightnessController brightness(&backlightDevice);
    retrodisc::DisplayPower displayPower;
    retrodisc::HelpScreen help;
    if (options.showHelp) help.show();
    retrodisc::ServiceScreen service;
    if (options.showService) service.show();
    if (options.brightnessLevel >= 0) brightness.setUiLevel(options.brightnessLevel);
    std::printf("[backlight] %s\n", brightness.backlightAvailable()
                                        ? backlightDevice.path().c_str()
                                        : "unavailable, UI brightness only");

    retrodisc::EventBus bus;
    bus.subscribe([](const retrodisc::Event& event) {
        switch (event.type) {
            case retrodisc::EventType::StateChanged:
                std::printf("[state] %s\n", std::string(retrodisc::toString(event.state)).c_str());
                break;
            case retrodisc::EventType::TrackChanged:
                std::printf("[track] %02d\n", event.track);
                break;
            case retrodisc::EventType::Error:
                std::printf("[error] %s\n", event.message.c_str());
                break;
        }
    });

    retrodisc::MockCDBackend cd;
    retrodisc::MockAudioBackend audio;
    audio.setRealtime(!options.fast);

    retrodisc::PlayerCore core(cd, audio, bus);
    core.setChunkFrames(retrodisc::kFramesPerSector * retrodisc::kSectorsPerSecond / 2);

    if (!cd.open()) {
        std::fprintf(stderr, "[error] cannot open the CD backend\n");
        return 1;
    }
    if (!audio.open(cfg.audioDevice)) {
        std::fprintf(stderr, "[error] cannot open audio device '%s'\n", cfg.audioDevice.c_str());
        return 1;
    }
    audio.setVolume(cfg.volume);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (options.render) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::fprintf(stderr, "[error] SDL_Init: %s\n", SDL_GetError());
            return 1;
        }
        window = SDL_CreateWindow("RetroDisc", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800,
                                  480, SDL_WINDOW_SHOWN);
        if (window != nullptr) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == nullptr && window != nullptr) {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        }
        if (renderer == nullptr) {
            std::fprintf(stderr, "[error] cannot create renderer: %s\n", SDL_GetError());
            if (window != nullptr) SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }

    if (options.autoDisc) {
        cd.insertDisc();
        std::printf("[disc] mock disc inserted\n");
    }

    if (!core.insertDisc()) {
        std::fprintf(stderr, "[error] %s\n", core.lastError().c_str());
        return 1;
    }
    std::printf("[toc] %d tracks, %s total, disc %s\n", core.trackCount(),
                retrodisc::formatTime(core.totalMs()).c_str(), core.disc().discId.c_str());
    for (int i = 1; i <= core.trackCount(); ++i) {
        const retrodisc::TrackInfo* track = core.disc().track(i);
        if (track == nullptr) continue;
        std::printf(
            "[toc] %02d %-20s %s\n", track->number, retrodisc::formatTrackTitle(*track).c_str(),
            retrodisc::formatTime(track->lengthSectors * 1000 / retrodisc::kSectorsPerSecond)
                .c_str());
    }

    const retrodisc::ResumePoint resume = retrodisc::loadResume(state);
    if (core.applyResume(resume)) {
        std::printf("[resume] restored track %02d at %s\n", core.trackNumber(),
                    retrodisc::formatTime(core.elapsedMs()).c_str());
    }

    if (!core.play()) {
        std::fprintf(stderr, "[error] %s\n", core.lastError().c_str());
        return 1;
    }

    const long long limitFrames = static_cast<long long>(options.seconds) * retrodisc::kSampleRate;
    long long playedFrames = 0;
    int lastPrintedSecond = -1;
    bool quit = false;

    auto applyAction = [&](retrodisc::Action action) {
        switch (action) {
            case retrodisc::Action::PlayPause:
                if (core.state() == retrodisc::PlayerState::Playing) {
                    core.pause();
                } else {
                    core.play();
                }
                break;
            case retrodisc::Action::Stop:
                core.stop();
                break;
            case retrodisc::Action::Next:
                core.next();
                break;
            case retrodisc::Action::Prev:
                core.prev();
                break;
            case retrodisc::Action::VolUp:
                cfg.volume = std::min(1.0f, cfg.volume + 0.05f);
                audio.setVolume(cfg.volume);
                std::printf("[vol] %d%%\n", static_cast<int>(cfg.volume * 100.0f + 0.5f));
                break;
            case retrodisc::Action::VolDown:
                cfg.volume = std::max(0.0f, cfg.volume - 0.05f);
                audio.setVolume(cfg.volume);
                std::printf("[vol] %d%%\n", static_cast<int>(cfg.volume * 100.0f + 0.5f));
                break;
            case retrodisc::Action::Theme: {
                const retrodisc::Theme& theme = themes.cycle();
                std::printf("[theme] %s (%s)\n", theme.name.c_str(), themes.currentId().c_str());
                break;
            }
            case retrodisc::Action::Eject:
                core.eject();
                break;
            case retrodisc::Action::BrightnessDown:
                brightness.uiDown();
                std::printf("[brightness] ui %d%%\n", brightness.uiPercent());
                break;
            case retrodisc::Action::BrightnessUp:
                brightness.uiUp();
                std::printf("[brightness] ui %d%%\n", brightness.uiPercent());
                break;
            case retrodisc::Action::DisplayToggle:
                displayPower.toggle();
                std::printf("[display] %s\n", displayPower.on() ? "on" : "off");
                break;
            case retrodisc::Action::Spectrum:
                options.spectrum = !options.spectrum;
                std::printf("[spectrum] %s\n", options.spectrum ? "on" : "off");
                break;
            case retrodisc::Action::Mode: {
                const retrodisc::PlayMode next = retrodisc::nextPlayMode(core.playMode());
                core.setPlayMode(next);
                std::printf("[mode] %s\n", std::string(retrodisc::toString(next)).c_str());
                break;
            }
            case retrodisc::Action::Random:
                core.setPlayMode(retrodisc::PlayMode::Random);
                std::printf("[mode] RANDOM\n");
                break;
            case retrodisc::Action::Repeat: {
                const retrodisc::PlayMode current = core.playMode();
                const retrodisc::PlayMode next =
                    current == retrodisc::PlayMode::RepeatAll   ? retrodisc::PlayMode::RepeatOne
                    : current == retrodisc::PlayMode::RepeatOne ? retrodisc::PlayMode::Normal
                                                                : retrodisc::PlayMode::RepeatAll;
                core.setPlayMode(next);
                std::printf("[mode] %s\n", std::string(retrodisc::toString(next)).c_str());
                break;
            }
            case retrodisc::Action::Help:
                if (help.visible()) {
                    help.hide();
                } else {
                    service.hide();
                    help.show();
                }
                break;
            case retrodisc::Action::Service:
                if (service.visible()) {
                    service.hide();
                } else {
                    help.hide();
                    service.show();
                }
                break;
            case retrodisc::Action::Quit:
                quit = true;
                break;
            default:
                break;
        }
    };

    while (!quit && core.state() == retrodisc::PlayerState::Playing && playedFrames < limitFrames) {
        const int frames = core.tick();
        if (frames <= 0 && core.state() != retrodisc::PlayerState::Playing) break;
        playedFrames += frames;

        const int second = static_cast<int>(playedFrames / retrodisc::kSampleRate);
        if (second != lastPrintedSecond) {
            lastPrintedSecond = second;
            std::printf("[play] track %02d  %s / %s  vol %d%%\n", core.trackNumber(),
                        retrodisc::formatTime(core.elapsedMs()).c_str(),
                        retrodisc::formatTime(core.trackTotalMs()).c_str(),
                        static_cast<int>(cfg.volume * 100.0f + 0.5f));
        }

        if (renderer != nullptr) {
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_QUIT) quit = true;
                if (event.type == SDL_KEYDOWN) {
                    const bool escape = event.key.keysym.sym == SDLK_ESCAPE;
                    const char* name = SDL_GetKeyName(event.key.keysym.sym);
                    const retrodisc::Action action =
                        name != nullptr ? keys.resolve(name) : retrodisc::Action::None;

                    if (service.visible()) {
                        if (escape || action == retrodisc::Action::Service) {
                            service.hide();
                        } else if (action == retrodisc::Action::VolUp) {
                            service.scrollBy(-1);
                        } else if (action == retrodisc::Action::VolDown) {
                            service.scrollBy(1);
                        }
                    } else if (help.visible()) {
                        // In help: UP/DOWN scroll, H or ESC closes. Nothing else leaks through.
                        if (escape || action == retrodisc::Action::Help) {
                            help.hide();
                        } else if (action == retrodisc::Action::VolUp) {
                            help.scrollBy(-1);
                        } else if (action == retrodisc::Action::VolDown) {
                            help.scrollBy(1);
                        }
                    } else if (escape) {
                        quit = true;
                    } else if (!displayPower.on()) {
                        displayPower.wake();  // first key only wakes the screen
                        std::printf("[display] on\n");
                    } else {
                        applyAction(action);
                    }
                }
            }
            if (displayPower.on()) {
                const retrodisc::Theme& active = brightness.apply(themes.current());
                if (service.visible()) {
                    retrodisc::renderServiceScreen(renderer, service, active);
                } else if (help.visible()) {
                    retrodisc::renderHelpScreen(renderer, help, active);
                } else {
                    retrodisc::renderMainScreen(renderer, makeModel(core, cfg, options.spectrum),
                                                active);
                }
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
            }
            SDL_RenderPresent(renderer);
            if (!options.screenshot.empty()) {
                if (saveFrame(renderer, options.screenshot)) {
                    std::printf("[ui] frame saved to %s\n", options.screenshot.c_str());
                } else {
                    std::printf("[ui] failed to save frame: %s\n", SDL_GetError());
                }
                options.screenshot.clear();
            }
        }
    }

    if (core.state() == retrodisc::PlayerState::Playing ||
        core.state() == retrodisc::PlayerState::Paused) {
        retrodisc::saveResume(state, core.resumePoint());
        std::printf("[resume] saved track %02d at %s\n", core.trackNumber(),
                    retrodisc::formatTime(core.elapsedMs()).c_str());
    }
    if (core.state() == retrodisc::PlayerState::Playing) core.stop();
    const double playedSeconds =
        static_cast<double>(playedFrames) / static_cast<double>(retrodisc::kSampleRate);

    if (core.eject()) std::printf("[eject] tray opened, push it back by hand\n");

    if (renderer != nullptr) SDL_DestroyRenderer(renderer);
    if (window != nullptr) SDL_DestroyWindow(window);
    if (options.render) SDL_Quit();

    std::printf("[done] played %.1f s of audio, final state %s\n", playedSeconds,
                std::string(retrodisc::toString(core.state())).c_str());
    return 0;
}
