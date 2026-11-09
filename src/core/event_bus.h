#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "core/player_state.h"

namespace retrodisc {

enum class EventType {
    StateChanged,
    Error,
};

struct Event {
    EventType type = EventType::StateChanged;
    PlayerState state = PlayerState::NoDisc;
    std::string message;

    static Event stateChanged(PlayerState state);
    static Event error(std::string message);
};

// Synchronous in-order dispatch. Handlers are called in subscription order.
// Adding or removing subscriptions during publish() is safe: the dispatch list is
// snapshotted, and handlers removed before their turn are skipped. Handlers added
// during a dispatch do not receive the event being dispatched.
class EventBus {
   public:
    using Handler = std::function<void(const Event&)>;

    int subscribe(Handler handler);
    bool unsubscribe(int id);
    bool isSubscribed(int id) const;
    void publish(const Event& event);
    std::size_t subscriberCount() const {
        return entries_.size();
    }

   private:
    struct Entry {
        int id = 0;
        Handler handler;
    };

    std::vector<Entry> entries_;
    int nextId_ = 1;
};

}  // namespace retrodisc
