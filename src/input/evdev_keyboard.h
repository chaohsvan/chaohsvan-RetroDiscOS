#pragma once

#include <libevdev/libevdev.h>

#include <string>

#include "input/key_mapping.h"

namespace retrodisc {

// Reads a keyboard (or the power button) straight from /dev/input/eventX.
// This is the input path used on the real device: there is no X server and no
// SDL window to steal key events from (docs/decisions.md D3).
class EvdevKeyboard {
   public:
    explicit EvdevKeyboard(const KeyMap& keys);
    ~EvdevKeyboard();

    EvdevKeyboard(const EvdevKeyboard&) = delete;
    EvdevKeyboard& operator=(const EvdevKeyboard&) = delete;

    bool open(const std::string& path);
    void close();
    bool isOpen() const {
        return dev_ != nullptr;
    }
    const std::string& path() const {
        return path_;
    }
    const std::string& lastError() const {
        return lastError_;
    }
    const std::string& name() const {
        return name_;
    }

    // Waits up to timeoutMs for a key press and returns its action.
    // Action::None means "nothing mapped happened" (timeout, release, key-up).
    Action poll(int timeoutMs);

   private:
    KeyMap keys_;
    int fd_ = -1;
    libevdev* dev_ = nullptr;
    std::string path_;
    std::string name_;
    std::string lastError_;
};

}  // namespace retrodisc
