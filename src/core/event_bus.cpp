#include "core/event_bus.h"

#include <utility>

namespace retrodisc {

Event Event::stateChanged(PlayerState state) {
    Event event;
    event.type = EventType::StateChanged;
    event.state = state;
    return event;
}

Event Event::trackChanged(int track) {
    Event event;
    event.type = EventType::TrackChanged;
    event.track = track;
    return event;
}

Event Event::error(std::string message) {
    Event event;
    event.type = EventType::Error;
    event.message = std::move(message);
    return event;
}

int EventBus::subscribe(Handler handler) {
    const int id = nextId_++;
    entries_.push_back(Entry{id, std::move(handler)});
    return id;
}

bool EventBus::unsubscribe(int id) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->id == id) {
            entries_.erase(it);
            return true;
        }
    }
    return false;
}

bool EventBus::isSubscribed(int id) const {
    for (const auto& entry : entries_) {
        if (entry.id == id) return true;
    }
    return false;
}

void EventBus::publish(const Event& event) {
    const std::vector<Entry> snapshot = entries_;
    for (const auto& entry : snapshot) {
        if (!isSubscribed(entry.id)) continue;  // removed earlier in this dispatch
        if (entry.handler) entry.handler(event);
    }
}

}  // namespace retrodisc
