#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "core/state_store.h"
#include "display/theme.h"
#include "display/theme_manager.h"
#include "test_util.h"

using retrodisc::StateStore;
using retrodisc::Theme;
using retrodisc::ThemeManager;

RD_TEST(registry_has_all_five_themes) {
    const auto& themes = retrodisc::allThemes();
    RD_CHECK_EQ(themes.size(), static_cast<std::size_t>(5));

    const char* expected[] = {"digital-hifi", "classic-vfd", "amber", "vintage-analog", "off"};
    for (std::size_t i = 0; i < themes.size(); ++i) {
        RD_CHECK_EQ(themes[i].id, std::string(expected[i]));
        RD_CHECK(!themes[i].name.empty());
    }
}

RD_TEST(each_theme_has_a_distinct_foreground) {
    const auto& themes = retrodisc::allThemes();
    for (std::size_t i = 0; i < themes.size(); ++i) {
        for (std::size_t j = i + 1; j < themes.size(); ++j) {
            const bool same = themes[i].foreground.r == themes[j].foreground.r &&
                              themes[i].foreground.g == themes[j].foreground.g &&
                              themes[i].foreground.b == themes[j].foreground.b;
            RD_CHECK(!same);
        }
    }
}

RD_TEST(lookup_and_fallback) {
    RD_CHECK(retrodisc::themeById("amber") != nullptr);
    RD_CHECK_EQ(retrodisc::themeById("amber")->name, std::string("AMBER"));
    RD_CHECK(retrodisc::themeById("nope") == nullptr);
    RD_CHECK_EQ(retrodisc::themeOrFallback("nope").id, retrodisc::defaultTheme().id);
}

RD_TEST(off_theme_is_flagged_and_black) {
    const Theme* off = retrodisc::themeById("off");
    RD_CHECK(off != nullptr);
    if (off == nullptr) return;
    RD_CHECK(off->displayOff);
    RD_CHECK_EQ(static_cast<int>(off->foreground.r + off->foreground.g + off->foreground.b), 0);
}

RD_TEST(next_theme_cycles_through_all_and_wraps) {
    std::string id = retrodisc::defaultTheme().id;
    std::vector<std::string> visited;
    for (std::size_t i = 0; i < retrodisc::allThemes().size(); ++i) {
        visited.push_back(id);
        id = retrodisc::nextThemeId(id);
    }
    RD_CHECK_EQ(visited.size(), retrodisc::allThemes().size());
    RD_CHECK_EQ(id, retrodisc::defaultTheme().id);  // wrapped around
    RD_CHECK_EQ(visited.front(), std::string("digital-hifi"));
}

RD_TEST(next_theme_from_unknown_id_returns_default) {
    RD_CHECK_EQ(retrodisc::nextThemeId("nonsense"), retrodisc::defaultTheme().id);
}

RD_TEST(manager_rejects_unknown_theme) {
    ThemeManager manager("amber");
    RD_CHECK_EQ(manager.currentId(), std::string("amber"));
    RD_CHECK(!manager.set("plaid"));
    RD_CHECK_EQ(manager.currentId(), std::string("amber"));
}

RD_TEST(manager_cycle_and_persist) {
    const std::string path = "/tmp/retrodisc_theme_state.conf";
    std::remove(path.c_str());

    {
        StateStore store(path);
        store.load();
        ThemeManager manager("digital-hifi", &store);

        RD_CHECK_EQ(manager.cycle().id, std::string("classic-vfd"));
        RD_CHECK_EQ(manager.cycle().id, std::string("amber"));

        StateStore reloaded(path);
        RD_CHECK(reloaded.load());
        RD_CHECK_EQ(reloaded.get("theme"), std::string("amber"));
        RD_CHECK_EQ(ThemeManager::initialIdFromStore(reloaded, "digital-hifi"),
                    std::string("amber"));
    }
    std::remove(path.c_str());
}

RD_TEST(manager_recovers_from_corrupt_state) {
    const std::string path = "/tmp/retrodisc_theme_state_bad.conf";
    {
        StateStore store(path);
        store.set("theme", "neon-disco");
        RD_CHECK(store.save());
    }
    StateStore loaded(path);
    RD_CHECK(loaded.load());
    RD_CHECK_EQ(ThemeManager::initialIdFromStore(loaded, "digital-hifi"),
                std::string("digital-hifi"));
    std::remove(path.c_str());
}

RD_TEST(manager_with_unknown_initial_falls_back) {
    ThemeManager manager("does-not-exist");
    RD_CHECK_EQ(manager.currentId(), retrodisc::defaultTheme().id);
}

RD_TEST(state_store_round_trip) {
    const std::string path = "/tmp/retrodisc_state_store.conf";
    std::remove(path.c_str());
    {
        StateStore store(path);
        store.set("theme", "amber");
        store.set("resume.track", "3");
        RD_CHECK(store.save());
    }
    StateStore loaded(path);
    RD_CHECK(loaded.load());
    RD_CHECK_EQ(loaded.get("theme"), std::string("amber"));
    RD_CHECK_EQ(loaded.get("resume.track"), std::string("3"));
    RD_CHECK_EQ(loaded.get("missing", "fallback"), std::string("fallback"));
    loaded.remove("theme");
    RD_CHECK_EQ(loaded.get("theme", "none"), std::string("none"));
    std::remove(path.c_str());
}

RD_TEST(missing_state_file_is_not_an_error) {
    StateStore store("/tmp/retrodisc_no_such_state_file.conf");
    RD_CHECK(!store.load());
    RD_CHECK_EQ(store.get("theme", "digital-hifi"), std::string("digital-hifi"));
}

RD_TEST_MAIN()
