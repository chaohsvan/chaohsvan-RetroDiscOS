#pragma once

#include <string>

namespace retrodisc {

enum class PowerKeyResult {
    None,
    ShortPress,
    LongPress,
};

// Short/long press detection, kept free of any device I/O so it can be tested
// with synthetic timestamps. The firmware keeps handling a real long hold (hard
// power-off); this only decides what the software does first.
class PowerKeyState {
  public:
    explicit PowerKeyState(int longPressMs = 1500);

    PowerKeyResult onPress(long long nowMs);
    PowerKeyResult onRelease(long long nowMs);
    // Call while the key may still be held: reports LongPress once the threshold
    // is crossed, so the UI can react before the user lets go.
    PowerKeyResult onTick(long long nowMs);

    bool isPressed() const { return pressed_; }
    bool longReported() const { return longReported_; }
    int longPressMs() const { return longPressMs_; }

  private:
    int longPressMs_;
    bool pressed_ = false;
    bool longReported_ = false;
    long long pressStartMs_ = 0;
};

}  // namespace retrodisc
