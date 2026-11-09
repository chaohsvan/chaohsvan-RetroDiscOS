#include <vector>

#include "core/event_bus.h"
#include "core/player_state.h"
#include "test_util.h"

using retrodisc::EventBus;
using retrodisc::PlayerEvent;
using retrodisc::PlayerState;
using retrodisc::PlayerStateMachine;

namespace {

// Drive a machine from NO_DISC to the requested state through legal transitions.
void driveTo(PlayerStateMachine& sm, PlayerState wanted) {
    if (wanted == PlayerState::NoDisc) return;
    sm.handle(PlayerEvent::DiscInserted);  // -> DISC_DETECTED
    if (wanted == PlayerState::DiscDetected) return;
    sm.handle(PlayerEvent::TocRead);  // -> DISC_READY
    if (wanted == PlayerState::DiscReady) return;
    if (wanted == PlayerState::Ejecting) {
        sm.handle(PlayerEvent::Eject);
        return;
    }
    sm.handle(PlayerEvent::Play);  // -> PLAYING
    if (wanted == PlayerState::Playing) return;
    sm.handle(PlayerEvent::Pause);  // -> PAUSED
    if (wanted == PlayerState::Paused) return;
    sm.handle(PlayerEvent::Stop);  // -> STOPPED
}

const PlayerState kAllStates[] = {
    PlayerState::NoDisc, PlayerState::DiscDetected, PlayerState::DiscReady, PlayerState::Playing,
    PlayerState::Paused, PlayerState::Stopped,      PlayerState::Ejecting,
};

const PlayerEvent kAllEvents[] = {
    PlayerEvent::DiscInserted, PlayerEvent::TocRead, PlayerEvent::TocFailed,
    PlayerEvent::DiscRemoved,  PlayerEvent::Play,    PlayerEvent::Pause,
    PlayerEvent::Stop,         PlayerEvent::Eject,   PlayerEvent::EjectDone,
};

}  // namespace

RD_TEST(initial_state_is_no_disc) {
    PlayerStateMachine sm;
    RD_CHECK(sm.state() == PlayerState::NoDisc);
}

RD_TEST(full_happy_path) {
    PlayerStateMachine sm;
    RD_CHECK(sm.handle(PlayerEvent::DiscInserted));
    RD_CHECK(sm.state() == PlayerState::DiscDetected);
    RD_CHECK(sm.handle(PlayerEvent::TocRead));
    RD_CHECK(sm.state() == PlayerState::DiscReady);
    RD_CHECK(sm.handle(PlayerEvent::Play));
    RD_CHECK(sm.state() == PlayerState::Playing);
    RD_CHECK(sm.handle(PlayerEvent::Pause));
    RD_CHECK(sm.state() == PlayerState::Paused);
    RD_CHECK(sm.handle(PlayerEvent::Play));
    RD_CHECK(sm.state() == PlayerState::Playing);
    RD_CHECK(sm.handle(PlayerEvent::Stop));
    RD_CHECK(sm.state() == PlayerState::Stopped);
    RD_CHECK(sm.handle(PlayerEvent::Eject));
    RD_CHECK(sm.state() == PlayerState::Ejecting);
    RD_CHECK(sm.handle(PlayerEvent::EjectDone));
    RD_CHECK(sm.state() == PlayerState::NoDisc);
}

RD_TEST(toc_failure_returns_to_no_disc) {
    PlayerStateMachine sm;
    sm.handle(PlayerEvent::DiscInserted);
    RD_CHECK(sm.handle(PlayerEvent::TocFailed));
    RD_CHECK(sm.state() == PlayerState::NoDisc);
}

RD_TEST(illegal_transitions_are_rejected_and_keep_state) {
    PlayerStateMachine sm;  // NO_DISC
    RD_CHECK(!sm.handle(PlayerEvent::Play));
    RD_CHECK(!sm.handle(PlayerEvent::Pause));
    RD_CHECK(!sm.handle(PlayerEvent::Stop));
    RD_CHECK(!sm.handle(PlayerEvent::Eject));
    RD_CHECK(!sm.handle(PlayerEvent::TocRead));
    RD_CHECK(sm.state() == PlayerState::NoDisc);

    sm.handle(PlayerEvent::DiscInserted);
    RD_CHECK(!sm.handle(PlayerEvent::DiscInserted));  // double insert
    RD_CHECK(!sm.handle(PlayerEvent::Play));          // must read TOC first
    RD_CHECK(sm.state() == PlayerState::DiscDetected);
}

RD_TEST(no_state_accepts_events_it_should_not) {
    for (PlayerState state : kAllStates) {
        for (PlayerEvent event : kAllEvents) {
            PlayerStateMachine sm;
            driveTo(sm, state);
            RD_CHECK(sm.state() == state);

            const bool accepted = sm.accepts(event);
            const bool handled = sm.handle(event);
            RD_CHECK_EQ(accepted, handled);
            if (!accepted) RD_CHECK(sm.state() == state);
        }
    }
}

RD_TEST(disc_removal_never_leaves_a_stale_disc) {
    const PlayerState withDisc[] = {
        PlayerState::DiscDetected, PlayerState::DiscReady, PlayerState::Playing,
        PlayerState::Paused,       PlayerState::Stopped,   PlayerState::Ejecting,
    };
    for (PlayerState state : withDisc) {
        PlayerStateMachine sm;
        driveTo(sm, state);
        RD_CHECK(sm.state() == state);
        RD_CHECK(sm.handle(PlayerEvent::DiscRemoved));
        RD_CHECK(sm.state() == PlayerState::NoDisc);
    }
}

RD_TEST(state_changes_are_published_to_the_bus) {
    EventBus bus;
    std::vector<PlayerState> seen;
    bus.subscribe([&seen](const retrodisc::Event& event) {
        if (event.type == retrodisc::EventType::StateChanged) seen.push_back(event.state);
    });

    PlayerStateMachine sm(bus);
    sm.handle(PlayerEvent::DiscInserted);
    sm.handle(PlayerEvent::TocRead);
    RD_CHECK_EQ(seen.size(), static_cast<std::size_t>(2));
    RD_CHECK(seen[0] == PlayerState::DiscDetected);
    RD_CHECK(seen[1] == PlayerState::DiscReady);
}

RD_TEST(rejected_events_are_not_published) {
    EventBus bus;
    int published = 0;
    bus.subscribe([&published](const retrodisc::Event&) { ++published; });

    PlayerStateMachine sm(bus);
    RD_CHECK(!sm.handle(PlayerEvent::Play));
    RD_CHECK_EQ(published, 0);
}

RD_TEST(state_machine_without_bus_does_not_crash) {
    PlayerStateMachine sm;
    RD_CHECK(sm.handle(PlayerEvent::DiscInserted));
    RD_CHECK(sm.state() == PlayerState::DiscDetected);
}

RD_TEST(names_are_stable) {
    RD_CHECK(retrodisc::toString(PlayerState::Playing) == "PLAYING");
    RD_CHECK(retrodisc::toString(PlayerEvent::Eject) == "Eject");
}

RD_TEST_MAIN()
