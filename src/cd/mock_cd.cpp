#include "cd/mock_cd.h"

#include <cmath>
#include <utility>

namespace retrodisc {
namespace {

constexpr double kTwoPi = 6.28318530717958647692;
constexpr double kMockToneHz = 1000.0;
constexpr double kMockAmplitude = 0.30;

int secondsToSectors(int seconds) {
    return seconds * kSectorsPerSecond;
}

}  // namespace

MockCDBackend::MockCDBackend(MockCDConfig config) : config_(config) {}

bool MockCDBackend::open() {
    if (config_.failOnOpen) return false;
    open_ = true;
    return true;
}

void MockCDBackend::close() {
    open_ = false;
    discPresent_ = false;
}

bool MockCDBackend::detectDisc() {
    return open_ && discPresent_;
}

void MockCDBackend::insertDisc() {
    discPresent_ = true;
}

void MockCDBackend::removeDisc() {
    discPresent_ = false;
}

bool MockCDBackend::eject() {
    if (!open_) return false;
    // Deliberately does not close anything again: the tray stays open (D4).
    discPresent_ = false;
    return true;
}

DiscInfo MockCDBackend::readTOC() {
    DiscInfo info;
    if (!detectDisc() || config_.failOnToc) return info;
    if (config_.trackCount <= 0) return info;

    info.valid = true;
    info.discId = "MOCK-DISC-0001";
    info.trackCount = config_.trackCount;

    int cursor = 0;
    for (int i = 1; i <= config_.trackCount; ++i) {
        const bool isLast = (i == config_.trackCount);
        const int seconds = isLast ? config_.lastTrackSeconds : config_.trackSeconds;

        TrackInfo track;
        track.number = i;
        track.startLba = cursor;
        track.lengthSectors = secondsToSectors(seconds);
        if (config_.withCdText && (i % 2 == 1)) {
            track.title = "Mock Track " + std::to_string(i);
        }
        info.tracks.push_back(std::move(track));
        cursor += track.lengthSectors;
    }

    info.totalSectors = cursor;
    return info;
}

int MockCDBackend::readFrames(int trackNumber, int startLba, int frameCount,
                              std::vector<int16_t>& out) {
    out.clear();
    if (failOnRead_) return -1;
    if (frameCount <= 0) return -1;

    const DiscInfo info = readTOC();
    if (!info.valid) return -1;

    const TrackInfo* track = info.track(trackNumber);
    if (track == nullptr) return -1;
    if (startLba < 0 || startLba >= track->lengthSectors) return -1;

    int frames = frameCount;
    const int remainingFrames = (track->lengthSectors - startLba) * kFramesPerSector;
    if (frames > remainingFrames) frames = remainingFrames;

    out.resize(static_cast<std::size_t>(frames) * kChannels);
    const long long baseFrame = static_cast<long long>(startLba) * kFramesPerSector;
    for (int i = 0; i < frames; ++i) {
        const double t = static_cast<double>(baseFrame + i) / static_cast<double>(kSampleRate);
        const double value = kMockAmplitude * 32767.0 * std::sin(kTwoPi * kMockToneHz * t);
        const int16_t sample = static_cast<int16_t>(std::lround(value));
        out[static_cast<std::size_t>(i) * kChannels] = sample;
        out[static_cast<std::size_t>(i) * kChannels + 1] = sample;
    }
    return frames;
}

}  // namespace retrodisc
