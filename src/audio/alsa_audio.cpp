#include "audio/alsa_audio.h"

#include <algorithm>
#include <cerrno>

namespace retrodisc {
namespace {

constexpr unsigned int kLatencyUs = 200000;  // 200 ms of buffering

}  // namespace

ALSAAudioBackend::~ALSAAudioBackend() { close(); }

void ALSAAudioBackend::setError(const std::string& message) { lastError_ = message; }

bool ALSAAudioBackend::open(const std::string& device) {
    if (device.empty()) {
        setError("empty ALSA device name");
        return false;
    }
    if (pcm_ != nullptr) close();

    int rc = snd_pcm_open(&pcm_, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        setError(std::string("snd_pcm_open: ") + snd_strerror(rc));
        pcm_ = nullptr;
        return false;
    }

    // 1 = let ALSA resample if the hardware cannot do 44.1 kHz exactly.
    rc = snd_pcm_set_params(pcm_, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, kChannels,
                            kSampleRate, 1, kLatencyUs);
    if (rc < 0) {
        setError(std::string("snd_pcm_set_params: ") + snd_strerror(rc));
        snd_pcm_close(pcm_);
        pcm_ = nullptr;
        return false;
    }

    snd_pcm_prepare(pcm_);
    device_ = device;
    framesWritten_ = 0;
    state_ = AudioState::Ready;
    lastError_.clear();
    return true;
}

void ALSAAudioBackend::close() {
    if (pcm_ != nullptr) {
        snd_pcm_drop(pcm_);  // never block on shutdown
        snd_pcm_close(pcm_);
        pcm_ = nullptr;
    }
    device_.clear();
    framesWritten_ = 0;
    state_ = AudioState::Closed;
}

bool ALSAAudioBackend::start() {
    if (pcm_ == nullptr) return false;
    if (state_ == AudioState::Running) return true;

    if (state_ == AudioState::Paused) {
        const int rc = snd_pcm_pause(pcm_, 0);
        if (rc < 0) {
            // Some devices cannot resume from pause; restart instead.
            snd_pcm_prepare(pcm_);
        }
        state_ = AudioState::Running;
        return true;
    }

    snd_pcm_prepare(pcm_);
    state_ = AudioState::Running;
    return true;
}

bool ALSAAudioBackend::pause() {
    if (state_ != AudioState::Running) return false;
    const int rc = snd_pcm_pause(pcm_, 1);
    if (rc < 0) setError(std::string("snd_pcm_pause: ") + snd_strerror(rc));
    state_ = AudioState::Paused;
    return true;
}

bool ALSAAudioBackend::stop() {
    if (pcm_ == nullptr) return false;
    snd_pcm_drop(pcm_);
    snd_pcm_prepare(pcm_);
    framesWritten_ = 0;
    state_ = AudioState::Ready;
    return true;
}

bool ALSAAudioBackend::setVolume(float volume) {
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

bool ALSAAudioBackend::recover(int error) {
    if (error == -EPIPE) {  // underrun: restart the stream
        snd_pcm_prepare(pcm_);
        return true;
    }
    if (error == -ESTRPIPE) {  // suspended by the system
        int rc = 0;
        while ((rc = snd_pcm_resume(pcm_)) == -EAGAIN) {
            // wait for the device to come back
        }
        if (rc < 0) snd_pcm_prepare(pcm_);
        return true;
    }
    if (error == -EAGAIN) return true;
    return false;
}

int ALSAAudioBackend::write(const int16_t* frames, int frameCount) {
    if (pcm_ == nullptr || state_ != AudioState::Running) return -1;
    if (frames == nullptr || frameCount <= 0) return -1;

    const int16_t* source = frames;
    if (volume_ < 1.0f) {
        const std::size_t samples = static_cast<std::size_t>(frameCount) * kChannels;
        scratch_.resize(samples);
        for (std::size_t i = 0; i < samples; ++i) {
            const float scaled = static_cast<float>(frames[i]) * volume_;
            scratch_[i] = static_cast<int16_t>(std::clamp(scaled, -32768.0f, 32767.0f));
        }
        source = scratch_.data();
    }

    int written = 0;
    int retries = 0;
    while (written < frameCount) {
        const int rc = snd_pcm_writei(pcm_, source + static_cast<std::size_t>(written) * kChannels,
                                      static_cast<snd_pcm_uframes_t>(frameCount - written));
        if (rc < 0) {
            if (!recover(rc) || ++retries > 4) {
                setError(std::string("snd_pcm_writei: ") + snd_strerror(rc));
                return written > 0 ? written : -1;
            }
            continue;
        }
        written += rc;
    }

    framesWritten_ += written;
    return written;
}

}  // namespace retrodisc
