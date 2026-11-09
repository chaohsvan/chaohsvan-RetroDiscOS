#include "input/power_key_state.h"

namespace retrodisc {

PowerKeyState::PowerKeyState(int longPressMs) : longPressMs_(longPressMs > 0 ? longPressMs : 1500) {}

PowerKeyResult PowerKeyState::onPress(long long nowMs) {
    if (pressed_) return PowerKeyResult::None;  // autorepeat / duplicate press
    pressed_ = true;
    longReported_ = false;
    pressStartMs_ = nowMs;
    return PowerKeyResult::None;
}

PowerKeyResult PowerKeyState::onRelease(long long nowMs) {
    if (!pressed_) return PowerKeyResult::None;
    pressed_ = false;
    const long long held = nowMs - pressStartMs_;

    if (longReported_) return PowerKeyResult::None;  // already handled while held
    if (held >= longPressMs_) return PowerKeyResult::LongPress;
    return PowerKeyResult::ShortPress;
}

PowerKeyResult PowerKeyState::onTick(long long nowMs) {
    if (!pressed_ || longReported_) return PowerKeyResult::None;
    if (nowMs - pressStartMs_ < longPressMs_) return PowerKeyResult::None;
    longReported_ = true;
    return PowerKeyResult::LongPress;
}

}  // namespace retrodisc
