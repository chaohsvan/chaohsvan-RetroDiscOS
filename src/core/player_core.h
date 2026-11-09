#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "audio/audio_interface.h"
#include "cd/cd_interface.h"
#include "core/event_bus.h"
#include "core/play_mode.h"
#include "core/player_state.h"

namespace retrodisc {

// Wires the state machine to the CD and audio backends. All hardware access
// goes through the injected interfaces; core knows nothing about SDL, ALSA or
// libcdio (AGENTS.md 4).
class PlayerCore {
   public:
    PlayerCore(ICDBackend& cd, IAudioBackend& audio, EventBus& bus);

    // Chunk size in frames; forced to a multiple of kFramesPerSector so every
    // read starts on a sector boundary. Smaller chunks = finer progress updates.
    void setChunkFrames(int frames);

    bool insertDisc();  // detect + read TOC, no audio yet
    void removeDisc();  // physical removal, valid in any state
    bool play();
    bool pause();
    bool stop();
    bool next();
    bool prev();
    bool eject();

    // Pulls one chunk from the CD and pushes it to the audio backend.
    // Returns the number of frames consumed (0 when not playing).
    int tick();

    PlayerState state() const {
        return state_.state();
    }
    const DiscInfo& disc() const {
        return disc_;
    }
    int trackNumber() const {
        return track_;
    }
    int trackCount() const {
        return disc_.trackCount;
    }
    const TrackInfo* currentTrack() const {
        return disc_.track(track_);
    }
    std::string currentTitle() const;
    void setPlayMode(PlayMode mode) {
        mode_ = mode;
    }
    PlayMode playMode() const {
        return mode_;
    }
    // Deterministic random mode for tests.
    void seedRandom(unsigned int seed) {
        rng_.seed(seed);
    }

    int elapsedMs() const;
    int trackTotalMs() const;
    int totalMs() const {
        return disc_.totalMillis();
    }
    bool hasDisc() const {
        return disc_.valid;
    }
    const std::string& lastError() const {
        return lastError_;
    }

   private:
    bool selectTrack(int number, bool keepPlaying);
    void handleTrackEnd();
    int randomTrackNumber();
    void publishError(std::string message);
    void finishDisc();

    ICDBackend& cd_;
    IAudioBackend& audio_;
    EventBus& bus_;
    PlayerStateMachine state_;

    DiscInfo disc_;
    int track_ = 0;
    long long elapsedFrames_ = 0;
    int chunkFrames_ = kFramesPerSector * 10;
    PlayMode mode_ = PlayMode::Normal;
    std::mt19937 rng_{0x5EEDu};
    std::string lastError_;
    std::vector<int16_t> buffer_;
};

}  // namespace retrodisc
