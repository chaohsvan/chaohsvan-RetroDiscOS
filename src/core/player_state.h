#pragma once

#include <string_view>

namespace retrodisc {

class EventBus;

// Single source of truth for playback state (see AGENTS.md section 5.2).
// Every legal transition is listed in player_state.cpp; anything else returns false.
enum class PlayerState {
    NoDisc,
    DiscDetected,
    DiscReady,
    Playing,
    Paused,
    Stopped,
    Ejecting,
};

enum class PlayerEvent {
    DiscInserted,
    TocRead,
    TocFailed,
    DiscRemoved,
    Play,
    Pause,
    Stop,
    Eject,
    EjectDone,
};

std::string_view toString(PlayerState state);
std::string_view toString(PlayerEvent event);

class PlayerStateMachine {
   public:
    PlayerStateMachine() = default;
    explicit PlayerStateMachine(EventBus& bus);

    PlayerState state() const {
        return state_;
    }

    // Returns false and leaves the state untouched when the event is not legal
    // in the current state. Never asserts, never throws.
    bool handle(PlayerEvent event);

    // Pure query: would handle() accept the event right now?
    bool accepts(PlayerEvent event) const;

   private:
    static bool isLegal(PlayerState from, PlayerEvent event);
    static PlayerState target(PlayerState from, PlayerEvent event);

    PlayerState state_ = PlayerState::NoDisc;
    EventBus* bus_ = nullptr;
};

}  // namespace retrodisc
