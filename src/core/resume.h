#pragma once

#include <string>

namespace retrodisc {

class StateStore;

// PLAY-008: remember where playback stopped, but only for the same disc.
// Matching uses the TOC-derived disc id, not the drive, so swapping discs while
// the player is off never resumes the wrong album.
struct ResumePoint {
    std::string discId;
    int track = 1;
    int positionMs = 0;

    bool valid() const {
        return !discId.empty();
    }
};

ResumePoint loadResume(const StateStore& store);
void saveResume(StateStore& store, const ResumePoint& point);
void clearResume(StateStore& store);

}  // namespace retrodisc
