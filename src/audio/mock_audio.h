#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "audio/audio_interface.h"

namespace retrodisc {

// Virtual audio device. Two sinks:
//   "null"        discard samples (CI)
//   "*.wav"       write a 16-bit PCM WAV file (manual listening)
//
// With realtime enabled, write() paces itself to the wall clock so that a
// 3:45 track really takes 3:45 — this is what makes progress display testable.
class MockAudioBackend : public IAudioBackend {
   public:
    MockAudioBackend() = default;
    ~MockAudioBackend() override;

    bool open(const std::string& device) override;
    void close() override;

    bool start() override;
    bool pause() override;
    bool stop() override;

    bool setVolume(float volume) override;
    float volume() const override {
        return volume_;
    }

    int write(const int16_t* frames, int frameCount) override;

    AudioState state() const override {
        return state_;
    }
    long long framesWritten() const override {
        return framesWritten_;
    }
    int sampleRate() const override {
        return kSampleRate;
    }
    int channels() const override {
        return kChannels;
    }

    void setRealtime(bool realtime) {
        realtime_ = realtime;
    }
    bool realtime() const {
        return realtime_;
    }
    const std::string& device() const {
        return device_;
    }

   private:
    void finalizeWav();
    void writeWavHeader();
    void pace();

    AudioState state_ = AudioState::Closed;
    std::string device_;
    std::ofstream file_;
    bool isWav_ = false;

    float volume_ = 1.0f;
    long long framesWritten_ = 0;
    long long dataBytes_ = 0;
    bool realtime_ = true;
    std::vector<int16_t> scratch_;

    std::chrono::steady_clock::time_point startTime_{};
};

}  // namespace retrodisc
