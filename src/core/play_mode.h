#pragma once

#include <string_view>

namespace retrodisc {

// PLAY-001..004. Normal plays the disc then stops; RepeatOne loops the current
// track; RepeatAll loops the whole disc; Random picks another track each time.
enum class PlayMode {
    Normal,
    RepeatAll,
    RepeatOne,
    Random,
};

std::string_view toString(PlayMode mode);

// F1 cycle order: Normal -> RepeatAll -> RepeatOne -> Random -> Normal.
PlayMode nextPlayMode(PlayMode mode);

}  // namespace retrodisc
