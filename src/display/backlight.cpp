#include "display/backlight.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace retrodisc {
namespace {

bool readInt(const std::string& path, int& out) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    in >> out;
    return in.good() || in.eof();
}

bool writeInt(const std::string& path, int value) {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << value;
    return out.good();
}

int clampPercent(int percent) {
    return std::max(0, std::min(100, percent));
}

}  // namespace

SysfsBacklight::SysfsBacklight(const std::string& root) {
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) return;

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        const std::string dir = entry.path().string();
        int maxRaw = 0;
        if (readInt(dir + "/max_brightness", maxRaw) && maxRaw > 0 &&
            std::filesystem::exists(dir + "/brightness", ec)) {
            dir_ = dir;
            maxRaw_ = maxRaw;
            available_ = true;
            return;
        }
    }
}

int SysfsBacklight::percent() const {
    if (!available_ || maxRaw_ <= 0) return 0;
    int raw = 0;
    if (!readInt(dir_ + "/brightness", raw)) return 0;
    return raw * 100 / maxRaw_;
}

bool SysfsBacklight::setPercent(int percent) {
    if (!available_ || maxRaw_ <= 0) return false;
    return writeInt(dir_ + "/brightness", clampPercent(percent) * maxRaw_ / 100);
}

int MockBacklight::percent() const {
    return maxRaw_ > 0 ? raw_ * 100 / maxRaw_ : 0;
}

bool MockBacklight::setPercent(int percent) {
    if (!available_ || maxRaw_ <= 0) return false;
    raw_ = clampPercent(percent) * maxRaw_ / 100;
    return true;
}

}  // namespace retrodisc
