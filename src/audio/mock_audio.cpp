#include "audio/mock_audio.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

namespace retrodisc {
namespace {

constexpr int kWavHeaderBytes = 44;

void putLE16(char* out, uint16_t value) {
    out[0] = static_cast<char>(value & 0xFF);
    out[1] = static_cast<char>((value >> 8) & 0xFF);
}

void putLE32(char* out, uint32_t value) {
    out[0] = static_cast<char>(value & 0xFF);
    out[1] = static_cast<char>((value >> 8) & 0xFF);
    out[2] = static_cast<char>((value >> 16) & 0xFF);
    out[3] = static_cast<char>((value >> 24) & 0xFF);
}

bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}  // namespace

MockAudioBackend::~MockAudioBackend() {
    close();
}

void MockAudioBackend::writeWavHeader() {
    char header[kWavHeaderBytes] = {};
    std::memcpy(header + 0, "RIFF", 4);
    putLE32(header + 4, 36);  // patched in finalizeWav()
    std::memcpy(header + 8, "WAVE", 4);
    std::memcpy(header + 12, "fmt ", 4);
    putLE32(header + 16, 16);  // PCM chunk size
    putLE16(header + 20, 1);   // PCM
    putLE16(header + 22, static_cast<uint16_t>(kChannels));
    putLE32(header + 24, static_cast<uint32_t>(kSampleRate));
    putLE32(header + 28, static_cast<uint32_t>(kSampleRate * kChannels * kBitsPerSample / 8));
    putLE16(header + 32, static_cast<uint16_t>(kChannels * kBitsPerSample / 8));  // block align
    putLE16(header + 34, static_cast<uint16_t>(kBitsPerSample));
    std::memcpy(header + 36, "data", 4);
    putLE32(header + 40, 0);  // patched in finalizeWav()
    file_.write(header, kWavHeaderBytes);
}

void MockAudioBackend::finalizeWav() {
    if (!isWav_ || !file_.is_open()) return;

    const auto dataBytes = static_cast<uint32_t>(dataBytes_);
    char patch[4];
    putLE32(patch, 36u + dataBytes);
    file_.seekp(4, std::ios::beg);
    file_.write(patch, 4);

    putLE32(patch, dataBytes);
    file_.seekp(40, std::ios::beg);
    file_.write(patch, 4);

    file_.flush();
    file_.close();
}

bool MockAudioBackend::open(const std::string& device) {
    if (device.empty()) return false;
    if (state_ != AudioState::Closed) close();

    device_ = device;
    framesWritten_ = 0;
    dataBytes_ = 0;

    if (endsWith(device, ".wav")) {
        file_.open(device, std::ios::binary | std::ios::trunc);
        if (!file_.is_open()) {
            device_.clear();
            return false;
        }
        isWav_ = true;
        writeWavHeader();
    }

    state_ = AudioState::Ready;
    return true;
}

void MockAudioBackend::close() {
    finalizeWav();
    if (file_.is_open()) file_.close();
    isWav_ = false;
    device_.clear();
    state_ = AudioState::Closed;
    framesWritten_ = 0;
    dataBytes_ = 0;
}

bool MockAudioBackend::start() {
    switch (state_) {
        case AudioState::Closed:
            return false;
        case AudioState::Ready:
            framesWritten_ = 0;
            startTime_ = std::chrono::steady_clock::now();
            state_ = AudioState::Running;
            return true;
        case AudioState::Paused: {
            const auto elapsed = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(static_cast<double>(framesWritten_) / kSampleRate));
            startTime_ = std::chrono::steady_clock::now() - elapsed;
            state_ = AudioState::Running;
            return true;
        }
        case AudioState::Running:
            return true;
    }
    return false;
}

bool MockAudioBackend::pause() {
    if (state_ == AudioState::Running) {
        state_ = AudioState::Paused;
        return true;
    }
    return state_ == AudioState::Paused;
}

bool MockAudioBackend::stop() {
    if (state_ == AudioState::Closed) return false;
    state_ = AudioState::Ready;
    framesWritten_ = 0;
    return true;
}

bool MockAudioBackend::setVolume(float volume) {
    if (!(volume >= 0.0f)) {  // also rejects NaN
        volume_ = 0.0f;
        return false;
    }
    if (volume > 1.0f) {
        volume_ = 1.0f;
        return false;
    }
    volume_ = volume;
    return true;
}

void MockAudioBackend::pace() {
    if (!realtime_) return;
    const auto target =
        startTime_ +
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(static_cast<double>(framesWritten_) / kSampleRate));
    std::this_thread::sleep_until(target);
}

int MockAudioBackend::write(const int16_t* frames, int frameCount) {
    if (state_ != AudioState::Running) return -1;
    if (frames == nullptr || frameCount <= 0) return -1;

    const std::size_t samples = static_cast<std::size_t>(frameCount) * kChannels;
    if (volume_ < 1.0f) {
        scratch_.resize(samples);
        for (std::size_t i = 0; i < samples; ++i) {
            const float scaled = static_cast<float>(frames[i]) * volume_;
            scratch_[i] = static_cast<int16_t>(std::clamp(scaled, -32768.0f, 32767.0f));
        }
        if (isWav_) {
            file_.write(reinterpret_cast<const char*>(scratch_.data()),
                        static_cast<std::streamsize>(samples * sizeof(int16_t)));
        }
    } else if (isWav_) {
        file_.write(reinterpret_cast<const char*>(frames),
                    static_cast<std::streamsize>(samples * sizeof(int16_t)));
    }

    if (isWav_) dataBytes_ += static_cast<long long>(samples * sizeof(int16_t));
    framesWritten_ += frameCount;
    pace();
    return frameCount;
}

}  // namespace retrodisc
