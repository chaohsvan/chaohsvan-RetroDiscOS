#pragma once

#include <cstdint>

namespace retrodisc {

inline constexpr int kGlyphWidth = 5;
inline constexpr int kGlyphHeight = 7;

// 5x7 bitmap font, no external font library (AGENTS.md 6: no new dependencies).
// Returns 7 rows of 5 bits each (bit 4 is the leftmost pixel), or nullptr for
// characters the font does not cover.
const uint8_t* glyphRows(char c);

}  // namespace retrodisc
