#pragma once

#include <string>

#include <libevdev/libevdev.h>

#include "input/power_key_state.h"

namespace retrodisc {

// Reads the ACPI power button straight from /dev/input. logind must be told to
// ignore the key (system/logind-retrodisc.conf), otherwise the machine powers
// off before this code sees anything.
class PowerKey {
  public:
    explicit PowerKey(int longPressMs = 1500);
    ~PowerKey();

    PowerKey(const PowerKey&) = delete;
    PowerKey& operator=(const PowerKey&) = delete;

    bool open(const std::string& path);
    void close();
    bool isOpen() const { return dev_ != nullptr; }

    // Waits up to timeoutMs; returns ShortPress/LongPress when detected.
    // Use a small timeout (e.g. 200 ms) so a held key is noticed promptly.
    PowerKeyResult poll(int timeoutMs);

    const std::string& path() const { return path_; }
    const std::string& name() const { return name_; }
    const std::string& lastError() const { return lastError_; }

  private:
    PowerKeyState state_;
    int fd_ = -1;
    libevdev* dev_ = nullptr;
    std::string path_;
    std::string name_;
    std::string lastError_;
};

// Finds the first /dev/input/event* device that advertises KEY_POWER.
// Returns an empty string when there is none. inputDir is injectable for tests.
std::string findPowerKeyDevice(const std::string& inputDir = "/dev/input");

}  // namespace retrodisc
