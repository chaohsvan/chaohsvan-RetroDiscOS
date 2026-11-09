#pragma once

namespace retrodisc {

// Shared PCM format for the whole player. Every backend must deliver this.
inline constexpr int kSampleRate = 44100;
inline constexpr int kChannels = 2;
inline constexpr int kBitsPerSample = 16;

}  // namespace retrodisc
