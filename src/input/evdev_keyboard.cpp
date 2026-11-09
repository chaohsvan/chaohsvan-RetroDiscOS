#include "input/evdev_keyboard.h"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "input/key_names.h"

namespace retrodisc {

EvdevKeyboard::EvdevKeyboard(const KeyMap& keys) : keys_(keys) {}

EvdevKeyboard::~EvdevKeyboard() {
    close();
}

bool EvdevKeyboard::open(const std::string& path) {
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

    fd_ = fd;
    dev_ = dev;
    path_ = path;
    name_ = libevdev_get_name(dev_) != nullptr ? libevdev_get_name(dev_) : "";
    lastError_.clear();
    return true;
}

void EvdevKeyboard::close() {
    if (dev_ != nullptr) {
        libevdev_free(dev_);
        dev_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    name_.clear();
    path_.clear();
}

Action EvdevKeyboard::poll(int timeoutMs) {
    if (dev_ == nullptr) return Action::None;

    struct pollfd pfd{};
    pfd.fd = fd_;
    pfd.events = POLLIN;
    const int ready = ::poll(&pfd, 1, timeoutMs);
    if (ready <= 0) return Action::None;
    if ((pfd.revents & POLLIN) == 0) return Action::None;

    input_event event{};
    int rc = libevdev_next_event(dev_, LIBEVDEV_READ_FLAG_NORMAL, &event);
    while (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
        if (event.type == EV_KEY && event.value == 1) {  // 1 = key press, 0 = release
            const Action action = keys_.resolve(keyNameFromCode(event.code));
            if (action != Action::None) return action;
        }
        rc = libevdev_next_event(dev_, LIBEVDEV_READ_FLAG_NORMAL, &event);
    }
    return Action::None;
}

}  // namespace retrodisc
