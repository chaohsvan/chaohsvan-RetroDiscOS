#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/audio_format.h"

namespace retrodisc {

// CD-DA framing constants (Red Book). The PCM format itself lives in
// core/audio_format.h so the audio layer does not depend on the CD layer.
inline constexpr int kFramesPerSector = 588;  // 2352 bytes / (2 ch * 2 bytes)
inline constexpr int kSectorsPerSecond = 75;

struct TrackInfo {
    int number = 0;    // 1-based, as printed on the disc
    int startLba = 0;  // relative to the start of track 1
    int lengthSectors = 0;
    std::string title;  // from CD-Text; empty when the disc has none
};

struct DiscInfo {
    bool valid = false;
    std::string discId;  // used for resume-session matching (PLAY-008)
    int trackCount = 0;
    int totalSectors = 0;
    std::vector<TrackInfo> tracks;

    int totalMillis() const;
    const TrackInfo* track(int number) const;
};

// "TRACK 01" fallback when CD-Text is missing (CD-004).
std::string formatTrackTitle(const TrackInfo& track);

// "03:45" style formatting.
std::string formatTime(int millis);

class ICDBackend {
   public:
    virtual ~ICDBackend() = default;

    virtual bool open() = 0;
    virtual void close() = 0;

    virtual bool detectDisc() = 0;

    // Returns an invalid DiscInfo on failure; never throws.
    virtual DiscInfo readTOC() = 0;

    // Reads audio frames from startLba within trackNumber.
    // out receives frameCount * kChannels interleaved samples.
    // Returns the number of frames written (may be clamped at the track end),
    // or -1 on failure.
    virtual int readFrames(int trackNumber, int startLba, int frameCount,
                           std::vector<int16_t>& out) = 0;

    // Software can only open the tray; closing it is a manual action (see
    // docs/decisions.md D4). There is deliberately no load().
    virtual bool eject() = 0;
};

}  // namespace retrodisc
