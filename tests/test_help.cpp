#include <string>

#include "test_util.h"
#include "ui/help_screen.h"

using retrodisc::HelpScreen;

RD_TEST(help_has_ten_sections) {
    const HelpScreen help;
    RD_CHECK_EQ(help.sectionCount(), static_cast<std::size_t>(10));
}

RD_TEST(every_section_has_content) {
    const HelpScreen help;
    for (const auto& section : help.sections()) {
        RD_CHECK(!section.title.empty());
        RD_CHECK(!section.lines.empty());
        for (const auto& line : section.lines) {
            RD_CHECK(!line.empty());
        }
    }
}

RD_TEST(all_help_text_is_ascii) {
    // The 5x7 font only covers ASCII; anything else would render as blanks.
    const HelpScreen help;
    for (const auto& section : help.sections()) {
        for (char c : section.title) RD_CHECK(static_cast<unsigned char>(c) < 128);
        for (const auto& line : section.lines) {
            for (char c : line) RD_CHECK(static_cast<unsigned char>(c) < 128);
        }
    }
}

RD_TEST(total_lines_matches_sections) {
    const HelpScreen help;
    int expected = 0;
    for (const auto& section : help.sections()) {
        expected += 1 + static_cast<int>(section.lines.size());
    }
    RD_CHECK_EQ(help.totalLines(), expected);
    RD_CHECK(help.totalLines() > HelpScreen::pageSize());
}

RD_TEST(visibility_toggles) {
    HelpScreen help;
    RD_CHECK(!help.visible());
    help.show();
    RD_CHECK(help.visible());
    help.hide();
    RD_CHECK(!help.visible());
    help.toggle();
    RD_CHECK(help.visible());
    help.toggle();
    RD_CHECK(!help.visible());
}

RD_TEST(show_resets_scroll) {
    HelpScreen help;
    help.scrollBy(5);
    RD_CHECK(help.scroll() > 0);
    help.show();
    RD_CHECK_EQ(help.scroll(), 0);
}

RD_TEST(scroll_clamps_at_both_ends) {
    HelpScreen help;
    RD_CHECK_EQ(help.scroll(), 0);

    help.scrollBy(-10);
    RD_CHECK_EQ(help.scroll(), 0);

    help.scrollBy(1000);
    RD_CHECK_EQ(help.scroll(), help.totalLines() - HelpScreen::pageSize());

    help.scrollToTop();
    RD_CHECK_EQ(help.scroll(), 0);

    help.scrollToBottom();
    RD_CHECK_EQ(help.scroll(), help.totalLines() - HelpScreen::pageSize());
}

RD_TEST(scrolling_one_line_at_a_time) {
    HelpScreen help;
    help.scrollBy(1);
    RD_CHECK_EQ(help.scroll(), 1);
    help.scrollBy(2);
    RD_CHECK_EQ(help.scroll(), 3);
    help.scrollBy(-1);
    RD_CHECK_EQ(help.scroll(), 2);
}

RD_TEST_MAIN()
