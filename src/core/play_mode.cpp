#include "core/play_mode.h"

namespace retrodisc {

std::string_view toString(PlayMode mode) {
    switch (mode) {
        case PlayMode::Normal:
            return "NORMAL";
        case PlayMode::RepeatAll:
            return "REPEAT ALL";
        case PlayMode::RepeatOne:
            return "REPEAT ONE";
        case PlayMode::Random:
            return "RANDOM";
    }
    return "UNKNOWN";
}

PlayMode nextPlayMode(PlayMode mode) {
    switch (mode) {
        case PlayMode::Normal:
            return PlayMode::RepeatAll;
        case PlayMode::RepeatAll:
            return PlayMode::RepeatOne;
        case PlayMode::RepeatOne:
            return PlayMode::Random;
        case PlayMode::Random:
            return PlayMode::Normal;
    }
    return PlayMode::Normal;
}

}  // namespace retrodisc
