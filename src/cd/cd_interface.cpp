#include "cd/cd_interface.h"

#include <cstdio>

namespace retrodisc {

int DiscInfo::totalMillis() const {
    return (totalSectors * 1000) / kSectorsPerSecond;
}

const TrackInfo* DiscInfo::track(int number) const {
    for (const auto& t : tracks) {
        if (t.number == number) return &t;
    }
    return nullptr;
}

std::string formatTrackTitle(const TrackInfo& track) {
    if (!track.title.empty()) return track.title;
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "TRACK %02d", track.number);
    return std::string(buffer);
}

std::string formatTime(int millis) {
    if (millis < 0) millis = 0;
    const int totalSeconds = millis / 1000;
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    return std::string(buffer);
}

}  // namespace retrodisc
