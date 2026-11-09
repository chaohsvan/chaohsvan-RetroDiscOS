#pragma once

#include <string>

namespace retrodisc {

// Hardware backlight, abstracted so the player degrades gracefully when the
// device does not exist (AGENTS.md 9). Never exits on failure.
class IBacklightDevice {
   public:
    virtual ~IBacklightDevice() = default;

    virtual bool available() const = 0;
    virtual int percent() const = 0;
    virtual bool setPercent(int percent) = 0;
};

// Used on machines with no backlight control (desktop, VM, CI).
class NullBacklight : public IBacklightDevice {
   public:
    bool available() const override {
        return false;
    }
    int percent() const override {
        return 0;
    }
    bool setPercent(int) override {
        return false;
    }
};

// Real device: /sys/class/backlight/<name>/{max_brightness,brightness}.
// The constructor probes for a usable entry and marks itself unavailable otherwise.
class SysfsBacklight : public IBacklightDevice {
   public:
    explicit SysfsBacklight(const std::string& root = "/sys/class/backlight");

    bool available() const override {
        return available_;
    }
    int percent() const override;
    bool setPercent(int percent) override;

    const std::string& path() const {
        return dir_;
    }

   private:
    std::string dir_;
    bool available_ = false;
    int maxRaw_ = 0;
};

// Test double.
class MockBacklight : public IBacklightDevice {
   public:
    explicit MockBacklight(int maxRaw = 100) : maxRaw_(maxRaw), available_(true) {}

    bool available() const override {
        return available_;
    }
    int percent() const override;
    bool setPercent(int percent) override;

    void setAvailable(bool available) {
        available_ = available;
    }
    int raw() const {
        return raw_;
    }

   private:
    int maxRaw_ = 100;
    int raw_ = 100;
    bool available_ = true;
};

}  // namespace retrodisc
