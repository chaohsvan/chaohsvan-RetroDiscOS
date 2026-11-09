#include <string>
#include <vector>

#include "core/event_bus.h"
#include "test_util.h"

using retrodisc::Event;
using retrodisc::EventBus;

RD_TEST(handlers_run_in_subscription_order) {
    EventBus bus;
    std::vector<int> order;
    bus.subscribe([&order](const Event&) { order.push_back(1); });
    bus.subscribe([&order](const Event&) { order.push_back(2); });
    bus.subscribe([&order](const Event&) { order.push_back(3); });

    bus.publish(Event::stateChanged(retrodisc::PlayerState::Playing));

    RD_CHECK_EQ(order.size(), static_cast<std::size_t>(3));
    RD_CHECK(order[0] == 1 && order[1] == 2 && order[2] == 3);
}

RD_TEST(unsubscribe_stops_delivery) {
    EventBus bus;
    int calls = 0;
    const int id = bus.subscribe([&calls](const Event&) { ++calls; });

    bus.publish(Event::error("first"));
    RD_CHECK_EQ(calls, 1);

    RD_CHECK(bus.unsubscribe(id));
    bus.publish(Event::error("second"));
    RD_CHECK_EQ(calls, 1);
    RD_CHECK_EQ(bus.subscriberCount(), static_cast<std::size_t>(0));

    RD_CHECK(!bus.unsubscribe(id));  // already gone
}

RD_TEST(unsubscribe_during_dispatch_is_safe) {
    EventBus bus;
    std::vector<int> order;
    int secondId = 0;

    bus.subscribe([&](const Event&) {
        order.push_back(1);
        bus.unsubscribe(secondId);  // remove the next handler mid-dispatch
    });
    secondId = bus.subscribe([&order](const Event&) { order.push_back(2); });
    bus.subscribe([&order](const Event&) { order.push_back(3); });

    bus.publish(Event::error("x"));

    RD_CHECK_EQ(order.size(), static_cast<std::size_t>(2));
    RD_CHECK(order[0] == 1);
    RD_CHECK(order[1] == 3);  // the removed handler was skipped
}

RD_TEST(subscribe_during_dispatch_does_not_receive_current_event) {
    EventBus bus;
    int lateCalls = 0;
    bus.subscribe([&bus, &lateCalls](const Event&) {
        bus.subscribe([&lateCalls](const Event&) { ++lateCalls; });
    });

    bus.publish(Event::error("first"));
    RD_CHECK_EQ(lateCalls, 0);

    bus.publish(Event::error("second"));
    RD_CHECK_EQ(lateCalls, 1);
}

RD_TEST(publish_without_subscribers_is_fine) {
    EventBus bus;
    bus.publish(Event::error("nobody is listening"));
    RD_CHECK_EQ(bus.subscriberCount(), static_cast<std::size_t>(0));
}

RD_TEST(error_event_carries_message) {
    EventBus bus;
    std::string captured;
    bus.subscribe([&captured](const Event& event) {
        if (event.type == retrodisc::EventType::Error) captured = event.message;
    });

    bus.publish(Event::error("drive not ready"));
    RD_CHECK(captured == "drive not ready");
}

RD_TEST_MAIN()
