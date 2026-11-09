#pragma once

#include <alsa/asoundlib.h>

#include <string>
#include <vector>

#include "audio/audio_interface.h"

namespace retrodisc {

// Real ALSA output. Device names come from config/player.conf ("default", "hw:1,0",
// "null", "plughw:..."). Volume is software gain by design (docs/decisions.md D2),
// so this backend is not bit-perfect.
class ALSAAudioBackend : public IAudioBackend {
public:
    ALSAAudioBackend() = default;
    ~ALSAAudioBackend() override;

    bool open(const std::string& device) override;
    void close() override;

    bool start() override;
    bool pause() override;
    bool stop() override;

    bool setVolume(float volume) override;
    float volume() const override { return volume_; }

    int write(const int16_t* frames, int frameCount) override;

    AudioState state() const override { return state_; }
    long long framesWritten() const override { return framesWritten_; }
    int sampleRate() const override { return kSampleRate; }
    int channels() const override { return kChannels; }

    const std::string& device() const { return device_; }
    const std::string& lastError() const { return lastError_; }

private:
    bool recover(int error);
    void setError(const std::string& message);

    snd_pcm_t* pcm_ = nullptr;
    AudioState state_ = AudioState::Closed;
    std::string device_;
    std::string lastError_;
    float volume_ = 1.0f;
    long long framesWritten_ = 0;
    std::vector<int16_t> scratch_;
};

}  // namespace retrodisc
