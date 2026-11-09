#include <set>
#include <string>

#include "system/diagnostics.h"
#include "test_util.h"
#include "ui/service_screen.h"

using retrodisc::Diagnostics;
using retrodisc::ServiceScreen;

RD_TEST(diagnostics_report_ten_items) {
    const Diagnostics diagnostics;
    const auto items = diagnostics.collect();

    RD_CHECK_EQ(static_cast<int>(items.size()), Diagnostics::kItemCount);
    std::set<std::string> labels;
    for (const auto& item : items) {
        RD_CHECK(!item.label.empty());
        RD_CHECK(!item.value.empty());
        labels.insert(item.label);
    }
    RD_CHECK_EQ(static_cast<int>(labels.size()), Diagnostics::kItemCount);  // labels are unique
}

RD_TEST(system_item_is_always_available) {
    const Diagnostics diagnostics;
    const auto items = diagnostics.collect();
    const auto& system = items.back();
    RD_CHECK_EQ(system.label, std::string("SYSTEM"));
    RD_CHECK(system.available);
    RD_CHECK(system.value.find("RetroDisc") != std::string::npos);
}

RD_TEST(cpu_is_reported_on_linux) {
    const Diagnostics diagnostics;
    const auto items = diagnostics.collect();
    for (const auto& item : items) {
        if (item.label != "CPU") continue;
        RD_CHECK(item.available);
        RD_CHECK(item.value.find("x") != std::string::npos);  // "<model> x<cores>"
    }
}

RD_TEST(missing_hardware_degrades_to_na) {
    const Diagnostics diagnostics("/tmp/retrodisc_no_such_root_at_all");
    const auto items = diagnostics.collect();
    RD_CHECK_EQ(static_cast<int>(items.size()), Diagnostics::kItemCount);

    for (const auto& item : items) {
        if (item.label == "SYSTEM") continue;
        RD_CHECK(!item.available);
        RD_CHECK_EQ(item.value, std::string("N/A"));
    }
}

RD_TEST(service_screen_toggles_and_refreshes) {
    ServiceScreen screen;
    RD_CHECK(!screen.visible());
    RD_CHECK_EQ(static_cast<int>(screen.items().size()), Diagnostics::kItemCount);

    screen.show();
    RD_CHECK(screen.visible());
    RD_CHECK_EQ(static_cast<int>(screen.items().size()), Diagnostics::kItemCount);

    screen.toggle();
    RD_CHECK(!screen.visible());
}

RD_TEST(service_screen_scroll_is_clamped) {
    ServiceScreen screen;
    screen.show();
    const int count = static_cast<int>(screen.items().size());

    RD_CHECK_EQ(screen.scroll(), 0);
    screen.scrollBy(-5);
    RD_CHECK_EQ(screen.scroll(), 0);

    screen.scrollBy(100);
    const int expected = count > ServiceScreen::pageSize() ? count - ServiceScreen::pageSize() : 0;
    RD_CHECK_EQ(screen.scroll(), expected);
}

RD_TEST(show_resets_scroll) {
    ServiceScreen screen;
    screen.scrollBy(3);
    screen.show();
    RD_CHECK_EQ(screen.scroll(), 0);
}

RD_TEST_MAIN()
