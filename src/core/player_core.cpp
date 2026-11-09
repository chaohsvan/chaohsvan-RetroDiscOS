#include "core/player_core.h"

#include <algorithm>
#include <utility>

namespace retrodisc {

PlayerCore::PlayerCore(ICDBackend& cd, IAudioBackend& audio, EventBus& bus)
    : cd_(cd), audio_(audio), bus_(bus), state_(bus) {}

void PlayerCore::setChunkFrames(int frames) {
    const int minimum = kFramesPerSector;
    if (frames < minimum) frames = minimum;
    chunkFrames_ = (frames / kFramesPerSector) * kFramesPerSector;
    if (chunkFrames_ < minimum) chunkFrames_ = minimum;
}

void PlayerCore::publishError(std::string message) {
    lastError_ = std::move(message);
    bus_.publish(Event::error(lastError_));
}

bool PlayerCore::insertDisc() {
    lastError_.clear();

    if (!cd_.detectDisc()) {
        publishError("no disc detected");
        return false;
    }
    if (!state_.handle(PlayerEvent::DiscInserted)) {
        publishError("a disc is already loaded");
        return false;
    }

    DiscInfo info = cd_.readTOC();
    if (!info.valid || info.trackCount <= 0) {
        state_.handle(PlayerEvent::TocFailed);
        publishError("failed to read the table of contents");
        return false;
    }

    disc_ = std::move(info);
    track_ = 1;
    elapsedFrames_ = 0;
    state_.handle(PlayerEvent::TocRead);
    bus_.publish(Event::trackChanged(track_));
    return true;
}

void PlayerCore::removeDisc() {
    audio_.stop();
    state_.handle(PlayerEvent::DiscRemoved);
    disc_ = DiscInfo{};
    track_ = 0;
    elapsedFrames_ = 0;
    buffer_.clear();
}

bool PlayerCore::play() {
    if (!disc_.valid) {
        publishError("no disc loaded");
        return false;
    }

    const PlayerState now = state_.state();
    if (now != PlayerState::DiscReady && now != PlayerState::Stopped &&
        now != PlayerState::Paused) {
        return false;
    }
    if (now != PlayerState::Paused) elapsedFrames_ = 0;

    if (!audio_.start()) {
        publishError("audio backend failed to start");
        return false;
    }
    return state_.handle(PlayerEvent::Play);
}

bool PlayerCore::pause() {
    if (state_.state() != PlayerState::Playing) return false;
    if (!audio_.pause()) return false;
    return state_.handle(PlayerEvent::Pause);
}

bool PlayerCore::stop() {
    const PlayerState now = state_.state();
    if (now != PlayerState::Playing && now != PlayerState::Paused) return false;
    audio_.stop();
    elapsedFrames_ = 0;
    return state_.handle(PlayerEvent::Stop);
}

bool PlayerCore::selectTrack(int number, bool keepPlaying) {
    if (!disc_.valid) return false;
    if (number < 1 || number > disc_.trackCount) return false;

    const bool wasPlaying = keepPlaying && state_.state() == PlayerState::Playing;
    if (wasPlaying) audio_.stop();

    track_ = number;
    elapsedFrames_ = 0;
    buffer_.clear();
    bus_.publish(Event::trackChanged(track_));

    if (wasPlaying && !audio_.start()) {
        publishError("audio backend failed to restart");
        return false;
    }
    return true;
}

bool PlayerCore::next() {
    if (!disc_.valid) return false;
    if (mode_ == PlayMode::Random) return selectTrack(randomTrackNumber(), true);
    if (track_ >= disc_.trackCount) {
        if (mode_ == PlayMode::RepeatAll) return selectTrack(1, true);
        return false;
    }
    return selectTrack(track_ + 1, true);
}

bool PlayerCore::prev() {
    if (!disc_.valid) return false;
    if (mode_ == PlayMode::Random) return selectTrack(randomTrackNumber(), true);
    if (track_ > 1) return selectTrack(track_ - 1, true);
    if (mode_ == PlayMode::RepeatAll && disc_.trackCount > 1) {
        return selectTrack(disc_.trackCount, true);
    }
    // At the first track, "previous" restarts it, like a real CD player.
    return selectTrack(1, true);
}

int PlayerCore::randomTrackNumber() {
    if (disc_.trackCount <= 1) return 1;
    std::uniform_int_distribution<int> dist(1, disc_.trackCount);
    int candidate = dist(rng_);
    if (candidate == track_) candidate = (candidate % disc_.trackCount) + 1;
    return candidate;
}

void PlayerCore::handleTrackEnd() {
    switch (mode_) {
        case PlayMode::RepeatOne:
            selectTrack(track_, true);
            return;
        case PlayMode::RepeatAll:
            selectTrack(track_ < disc_.trackCount ? track_ + 1 : 1, true);
            return;
        case PlayMode::Random:
            selectTrack(randomTrackNumber(), true);
            return;
        case PlayMode::Normal:
            if (track_ < disc_.trackCount) {
                selectTrack(track_ + 1, true);
            } else {
                finishDisc();
            }
            return;
    }
}

bool PlayerCore::eject() {
    const PlayerState now = state_.state();
    if (now == PlayerState::NoDisc || now == PlayerState::Ejecting) return false;

    audio_.stop();
    if (!state_.handle(PlayerEvent::Eject)) return false;

    // Only the opening action exists: the tray must be pushed back by hand (D4).
    cd_.eject();
    state_.handle(PlayerEvent::EjectDone);

    disc_ = DiscInfo{};
    track_ = 0;
    elapsedFrames_ = 0;
    buffer_.clear();
    return true;
}

void PlayerCore::finishDisc() {
    audio_.stop();
    elapsedFrames_ = 0;
    state_.handle(PlayerEvent::Stop);
}

int PlayerCore::tick() {
    if (state_.state() != PlayerState::Playing) return 0;

    const TrackInfo* track = disc_.track(track_);
    if (track == nullptr) return 0;

    const int startLba = static_cast<int>(elapsedFrames_ / kFramesPerSector);
    const int remainingFrames = (track->lengthSectors - startLba) * kFramesPerSector;
    if (remainingFrames <= 0) {
        finishDisc();
        return 0;
    }

    const int want = std::min(chunkFrames_, remainingFrames);
    const int got = cd_.readFrames(track_, startLba, want, buffer_);
    if (got <= 0) {
        publishError("failed to read audio from the disc");
        stop();
        return 0;
    }

    const int written = audio_.write(buffer_.data(), got);
    if (written <= 0) {
        publishError("failed to write audio to the output device");
        stop();
        return 0;
    }

    elapsedFrames_ += written;
    const long long trackFrames = static_cast<long long>(track->lengthSectors) * kFramesPerSector;
    if (elapsedFrames_ >= trackFrames) handleTrackEnd();
    return written;
}

std::string PlayerCore::currentTitle() const {
    const TrackInfo* track = disc_.track(track_);
    return track != nullptr ? formatTrackTitle(*track) : std::string();
}

int PlayerCore::elapsedMs() const {
    return static_cast<int>(elapsedFrames_ * 1000 / kSampleRate);
}

int PlayerCore::trackTotalMs() const {
    const TrackInfo* track = disc_.track(track_);
    if (track == nullptr) return 0;
    return track->lengthSectors * 1000 / kSectorsPerSecond;
}

}  // namespace retrodisc
