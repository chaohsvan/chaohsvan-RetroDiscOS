#pragma once

#include <string_view>

namespace retrodisc {

// Semantic version of the player, injected from CMakeLists.txt (project VERSION).
std::string_view version();

}  // namespace retrodisc
