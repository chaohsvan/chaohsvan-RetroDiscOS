#include "system/diagnostics.h"

#include <sys/statvfs.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/version.h"

namespace retrodisc {
namespace {

constexpr const char* kNotAvailable = "N/A";

std::string readFirstLine(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return {};
    std::string line;
    std::getline(in, line);
    return line;
}

bool readInt(const std::string& path, int& out) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    in >> out;
    return !in.fail();
}

int countEntries(const std::string& dir, const std::string& prefix = {}) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) return 0;
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        const std::string name = entry.path().filename().string();
        if (prefix.empty() || name.rfind(prefix, 0) == 0) ++count;
    }
    return count;
}

std::string humanGiB(unsigned long long bytes) {
    const double gib = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f GiB", gib);
    return std::string(buffer);
}

}  // namespace

Diagnostics::Diagnostics() : root_("") {}

Diagnostics::Diagnostics(std::string root) : root_(std::move(root)) {
    if (!root_.empty() && root_.back() == '/') root_.pop_back();
}

std::string Diagnostics::path(const std::string& relative) const {
    return root_ + relative;
}

std::vector<DiagnosticItem> Diagnostics::collect() const {
    std::vector<DiagnosticItem> items;
    items.reserve(kItemCount);

    // 1. CD-ROM
    {
        DiagnosticItem item{"CD-ROM", kNotAvailable, false};
        std::error_code ec;
        const std::string blockDir = path("/sys/class/block");
        if (std::filesystem::is_directory(blockDir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(blockDir, ec)) {
                const std::string name = entry.path().filename().string();
                if (name.rfind("sr", 0) != 0) continue;
                const std::string model =
                    readFirstLine(path("/sys/class/block/" + name + "/device/model"));
                if (!model.empty()) {
                    item.value = name + " " + model;
                    item.available = true;
                    break;
                }
            }
        }
        items.push_back(item);
    }

    // 2. Audio
    {
        DiagnosticItem item{"AUDIO", kNotAvailable, false};
        const std::string cards = readFirstLine(path("/proc/asound/cards"));
        std::error_code ec;
        if (std::filesystem::exists(path("/proc/asound/cards"), ec)) {
            item.value = cards.empty() ? "no card" : "card present";
            item.available = true;
        }
        items.push_back(item);
    }

    // 3. Keyboard / input devices
    {
        DiagnosticItem item{"INPUT", kNotAvailable, false};
        const int events = countEntries(path("/dev/input"), "event");
        if (events > 0) {
            item.value = std::to_string(events) + " event devices";
            item.available = true;
        }
        items.push_back(item);
    }

    // 4. Display
    {
        DiagnosticItem item{"DISPLAY", kNotAvailable, false};
        std::error_code ec;
        int connected = 0;
        const std::string drm = path("/sys/class/drm");
        if (std::filesystem::is_directory(drm, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(drm, ec)) {
                const std::string status = readFirstLine(entry.path().string() + "/status");
                if (status == "connected") ++connected;
            }
        }
        if (connected > 0) {
            item.value = std::to_string(connected) + " connected outputs";
            item.available = true;
        }
        items.push_back(item);
    }

    // 5. Backlight
    {
        DiagnosticItem item{"BACKLIGHT", kNotAvailable, false};
        const int count = countEntries(path("/sys/class/backlight"));
        if (count > 0) {
            item.value = std::to_string(count) + " devices";
            item.available = true;
        }
        items.push_back(item);
    }

    // 6. ACPI / power
    {
        DiagnosticItem item{"ACPI", kNotAvailable, false};
        const int supplies = countEntries(path("/sys/class/power_supply"));
        std::error_code ec;
        if (supplies > 0) {
            item.value = std::to_string(supplies) + " power supplies";
            item.available = true;
        } else if (std::filesystem::is_directory(path("/proc/acpi"), ec)) {
            item.value = "ACPI present";
            item.available = true;
        }
        items.push_back(item);
    }

    // 7. CPU
    {
        DiagnosticItem item{"CPU", kNotAvailable, false};
        std::ifstream cpuinfo(path("/proc/cpuinfo"));
        if (cpuinfo.is_open()) {
            int cores = 0;
            std::string line;
            std::string model;
            while (std::getline(cpuinfo, line)) {
                if (line.rfind("processor", 0) == 0) ++cores;
                if (model.empty() && line.rfind("model name", 0) == 0) {
                    const auto colon = line.find(':');
                    if (colon != std::string::npos) {
                        model = line.substr(colon + 1);
                        model.erase(0, model.find_first_not_of(' '));
                    }
                }
            }
            if (cores > 0) {
                item.value =
                    (model.empty() ? std::string("unknown") : model) + " x" + std::to_string(cores);
                item.available = true;
            }
        }
        items.push_back(item);
    }

    // 8. Fan (read-only monitoring; never controlled — AGENTS.md 9)
    {
        DiagnosticItem item{"FAN", kNotAvailable, false};
        std::error_code ec;
        int highest = -1;
        const std::string hwmon = path("/sys/class/hwmon");
        if (std::filesystem::is_directory(hwmon, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(hwmon, ec)) {
                for (int index = 1; index <= 4; ++index) {
                    int rpm = 0;
                    if (readInt(entry.path().string() + "/fan" + std::to_string(index) + "_input",
                                rpm)) {
                        highest = std::max(highest, rpm);
                    }
                }
            }
        }
        if (highest >= 0) {
            item.value = std::to_string(highest) + " RPM";
            item.available = true;
        }
        items.push_back(item);
    }

    // 9. Storage
    {
        DiagnosticItem item{"STORAGE", kNotAvailable, false};
        struct statvfs vfs{};
        const std::string target = root_.empty() ? "/" : root_;
        if (statvfs(target.c_str(), &vfs) == 0 && vfs.f_blocks > 0) {
            const unsigned long long total =
                static_cast<unsigned long long>(vfs.f_blocks) * vfs.f_frsize;
            const unsigned long long freeBytes =
                static_cast<unsigned long long>(vfs.f_bavail) * vfs.f_frsize;
            item.value = humanGiB(freeBytes) + " free of " + humanGiB(total);
            item.available = true;
        }
        items.push_back(item);
    }

    // 10. System
    {
        DiagnosticItem item{"SYSTEM", "RetroDisc " + std::string(version()), true};
        const std::string kernel = readFirstLine(path("/proc/sys/kernel/osrelease"));
        if (!kernel.empty()) item.value += " / kernel " + kernel;
        items.push_back(item);
    }

    return items;
}

}  // namespace retrodisc
