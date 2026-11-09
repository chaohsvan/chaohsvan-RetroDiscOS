#include <string>

#include "core/version.h"
#include "test_util.h"

RD_TEST(version_is_not_empty) {
    RD_CHECK(!retrodisc::version().empty());
}

RD_TEST(version_is_compiled_in) {
    RD_CHECK(std::string(retrodisc::version()) != "0.0.0-unknown");
}

RD_TEST(basic_arithmetic_sanity) {
    RD_CHECK_EQ(1 + 1, 2);
}

RD_TEST_MAIN()
