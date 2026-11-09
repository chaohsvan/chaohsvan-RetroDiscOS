#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "core/audio_format.h"

namespace retrodisc {

enum class AudioState {
    Closed,
    Ready,
    Running,
    Paused,
};

std::string_view toString(AudioState state);

class IAudioBackend {
   public:
    virtual ~IAudioBackend() = default;

    // device is backend-specific ("null" for the mock sink, a PCM name for ALSA).
    virtual bool open(const std::string& device) = 0;
    virtual void close() = 0;

    virtual bool start() = 0;
    virtual bool pause() = 0;
    virtual bool stop() = 0;

    // Software volume in [0.0, 1.0]. This is NOT a hardware mixer and therefore
    // not bit-perfect by design (see docs/decisions.md D2).
    virtual bool setVolume(float volume) = 0;
    virtual float volume() const = 0;

    // Returns the number of frames consumed, or -1 when not running.
    virtual int write(const int16_t* frames, int frameCount) = 0;

    virtual AudioState state() const = 0;
    virtual long long framesWritten() const = 0;
    virtual int sampleRate() const = 0;
    virtual int channels() const = 0;
};

}  // namespace retrodisc
