#include <SDL2/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "audio/mock_audio.h"
#include "cd/mock_cd.h"
#include "core/config.h"
#include "core/event_bus.h"
#include "core/player_core.h"
#include "core/version.h"
#include "ui/main_screen.h"

namespace {

struct Options {
    bool mock = false;
    bool autoDisc = false;
    bool render = false;
    bool fast = false;
    int seconds = 2;
    std::string configDir = "config";
    std::string screenshot;  // save the first rendered frame here
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
        "  --seconds N       How long to play before stopping (default 2)\n"
        "  --config DIR      Config directory (default ./config)\n"
        "  --screenshot FILE Save the first frame as a BMP (requires --render)\n"
        "  --fast            Do not pace audio to real time\n"
        "  --help            Show this help\n",
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

retrodisc::UiModel makeModel(const retrodisc::PlayerCore& core,
                             const retrodisc::PlayerConfig& cfg) {
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
    return model;
}

}  // namespace

// Reads the back buffer and writes it as a BMP, for headless UI review.
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
    if (const char* shot = valueOf(argc, argv, "--screenshot")) options.screenshot = shot;

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
    if (!audio.open(config.config.audioDevice)) {
        std::fprintf(stderr, "[error] cannot open audio device '%s'\n",
                     config.config.audioDevice.c_str());
        return 1;
    }
    audio.setVolume(config.config.volume);

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

    if (!core.play()) {
        std::fprintf(stderr, "[error] %s\n", core.lastError().c_str());
        return 1;
    }

    const long long limitFrames = static_cast<long long>(options.seconds) * retrodisc::kSampleRate;
    long long playedFrames = 0;
    int lastPrintedSecond = -1;
    bool quit = false;

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
                        static_cast<int>(config.config.volume * 100.0f + 0.5f));
        }

        if (renderer != nullptr) {
            SDL_Event event;
            while (SDL_PollEvent(&event) != 0) {
                if (event.type == SDL_QUIT) quit = true;
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) quit = true;
            }
            retrodisc::renderMainScreen(renderer, makeModel(core, config.config));
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
