#pragma once

namespace retrodisc {

// T10: the screen can be blanked while playback keeps running. Any key wakes it.
class DisplayPower {
   public:
    bool on() const {
        return on_;
    }
    void sleep() {
        on_ = false;
    }
    void wake() {
        on_ = true;
    }
    void toggle() {
        on_ = !on_;
    }

   private:
    bool on_ = true;
};

}  // namespace retrodisc
