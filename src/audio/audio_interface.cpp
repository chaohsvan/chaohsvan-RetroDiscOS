#include "audio/audio_interface.h"

namespace retrodisc {

std::string_view toString(AudioState state) {
    switch (state) {
        case AudioState::Closed:
            return "CLOSED";
        case AudioState::Ready:
            return "READY";
        case AudioState::Running:
            return "RUNNING";
        case AudioState::Paused:
            return "PAUSED";
    }
    return "UNKNOWN";
}

}  // namespace retrodisc
