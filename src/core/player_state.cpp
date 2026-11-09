#include "core/player_state.h"

#include <string_view>

#include "core/event_bus.h"

namespace retrodisc {
namespace {

using S = PlayerState;
using E = PlayerEvent;

struct Transition {
    S from;
    E event;
    S to;
};

// Every legal transition lives here. Nothing else may mutate PlayerState.
constexpr Transition kTransitions[] = {
    // Disc lifecycle
    {S::NoDisc, E::DiscInserted, S::DiscDetected},
    {S::DiscDetected, E::TocRead, S::DiscReady},
    {S::DiscDetected, E::TocFailed, S::NoDisc},

    // Playback
    {S::DiscReady, E::Play, S::Playing},
    {S::Playing, E::Pause, S::Paused},
    {S::Paused, E::Play, S::Playing},
    {S::Playing, E::Stop, S::Stopped},
    {S::Paused, E::Stop, S::Stopped},
    {S::Stopped, E::Play, S::Playing},

    // Eject: software may only open the tray (see docs/decisions.md D4).
    {S::DiscDetected, E::Eject, S::Ejecting},
    {S::DiscReady, E::Eject, S::Ejecting},
    {S::Playing, E::Eject, S::Ejecting},
    {S::Paused, E::Eject, S::Ejecting},
    {S::Stopped, E::Eject, S::Ejecting},
    {S::Ejecting, E::EjectDone, S::NoDisc},

    // Physical removal always wins: no state may keep a stale disc (PRD 3.1).
    {S::DiscDetected, E::DiscRemoved, S::NoDisc},
    {S::DiscReady, E::DiscRemoved, S::NoDisc},
    {S::Playing, E::DiscRemoved, S::NoDisc},
    {S::Paused, E::DiscRemoved, S::NoDisc},
    {S::Stopped, E::DiscRemoved, S::NoDisc},
    {S::Ejecting, E::DiscRemoved, S::NoDisc},
};

constexpr const Transition* find(S from, E event) {
    for (const auto& t : kTransitions) {
        if (t.from == from && t.event == event) return &t;
    }
    return nullptr;
}

}  // namespace

std::string_view toString(PlayerState state) {
    switch (state) {
        case PlayerState::NoDisc:
            return "NO_DISC";
        case PlayerState::DiscDetected:
            return "DISC_DETECTED";
        case PlayerState::DiscReady:
            return "DISC_READY";
        case PlayerState::Playing:
            return "PLAYING";
        case PlayerState::Paused:
            return "PAUSED";
        case PlayerState::Stopped:
            return "STOPPED";
        case PlayerState::Ejecting:
            return "EJECTING";
    }
    return "UNKNOWN";
}

std::string_view toString(PlayerEvent event) {
    switch (event) {
        case PlayerEvent::DiscInserted:
            return "DiscInserted";
        case PlayerEvent::TocRead:
            return "TocRead";
        case PlayerEvent::TocFailed:
            return "TocFailed";
        case PlayerEvent::DiscRemoved:
            return "DiscRemoved";
        case PlayerEvent::Play:
            return "Play";
        case PlayerEvent::Pause:
            return "Pause";
        case PlayerEvent::Stop:
            return "Stop";
        case PlayerEvent::Eject:
            return "Eject";
        case PlayerEvent::EjectDone:
            return "EjectDone";
    }
    return "Unknown";
}

PlayerStateMachine::PlayerStateMachine(EventBus& bus) : bus_(&bus) {}

bool PlayerStateMachine::isLegal(PlayerState from, PlayerEvent event) {
    return find(from, event) != nullptr;
}

PlayerState PlayerStateMachine::target(PlayerState from, PlayerEvent event) {
    const Transition* t = find(from, event);
    return t != nullptr ? t->to : from;
}

bool PlayerStateMachine::accepts(PlayerEvent event) const {
    return isLegal(state_, event);
}

bool PlayerStateMachine::handle(PlayerEvent event) {
    if (!isLegal(state_, event)) return false;

    state_ = target(state_, event);
    if (bus_ != nullptr) bus_->publish(Event::stateChanged(state_));
    return true;
}

}  // namespace retrodisc
