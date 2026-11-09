#include "input/power_key.h"

#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>

namespace retrodisc {
namespace {

long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

}  // namespace

PowerKey::PowerKey(int longPressMs) : state_(longPressMs) {}

PowerKey::~PowerKey() { close(); }

bool PowerKey::open(const std::string& path) {
    if (dev_ != nullptr) close();

    const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        lastError_ = std::string("open ") + path + ": " + std::strerror(errno);
        return false;
    }

    libevdev* dev = nullptr;
    const int rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0) {
        ::close(fd);
        lastError_ = std::string("libevdev_new_from_fd: ") + std::strerror(-rc);
        return false;
    }

    if (!libevdev_has_event_code(dev, EV_KEY, KEY_POWER)) {
        libevdev_free(dev);
        ::close(fd);
        lastError_ = path + " has no KEY_POWER capability";
        return false;
    }

    fd_ = fd;
    dev_ = dev;
    path_ = path;
    name_ = libevdev_get_name(dev_) != nullptr ? libevdev_get_name(dev_) : "";
    lastError_.clear();
    return true;
}

void PowerKey::close() {
    if (dev_ != nullptr) {
        libevdev_free(dev_);
        dev_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    path_.clear();
    name_.clear();
}

PowerKeyResult PowerKey::poll(int timeoutMs) {
    if (dev_ == nullptr) return PowerKeyResult::None;

    struct pollfd pfd {};
    pfd.fd = fd_;
    pfd.events = POLLIN;
    const int ready = ::poll(&pfd, 1, timeoutMs);

    if (ready > 0 && (pfd.revents & POLLIN) != 0) {
        input_event event {};
        int rc = libevdev_next_event(dev_, LIBEVDEV_READ_FLAG_NORMAL, &event);
        while (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            if (event.type == EV_KEY && event.code == KEY_POWER) {
                if (event.value == 1) {
                    const PowerKeyResult pressed = state_.onPress(nowMs());
                    if (pressed != PowerKeyResult::None) return pressed;
                } else if (event.value == 0) {
                    return state_.onRelease(nowMs());
                }
            }
            rc = libevdev_next_event(dev_, LIBEVDEV_READ_FLAG_NORMAL, &event);
        }
    }

    // Still held? Report the long press as soon as it crosses the threshold.
    return state_.onTick(nowMs());
}

std::string findPowerKeyDevice(const std::string& inputDir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(inputDir, ec)) return {};

    for (const auto& entry : std::filesystem::directory_iterator(inputDir, ec)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("event", 0) != 0) continue;

        const std::string path = entry.path().string();
        const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        libevdev* dev = nullptr;
        if (libevdev_new_from_fd(fd, &dev) < 0) {
            ::close(fd);
            continue;
        }
        const bool isPower = libevdev_has_event_code(dev, EV_KEY, KEY_POWER) != 0;
        libevdev_free(dev);
        ::close(fd);
        if (isPower) return path;
    }
    return {};
}

}  // namespace retrodisc
