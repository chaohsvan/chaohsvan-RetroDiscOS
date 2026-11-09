#include "core/state_store.h"

#include <fstream>
#include <sstream>

namespace retrodisc {
namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

}  // namespace

StateStore::StateStore(std::string path) : path_(std::move(path)) {}

bool StateStore::load() {
    values_.clear();
    std::ifstream in(path_);
    if (!in.is_open()) return false;  // first run: not an error

    std::string line;
    while (std::getline(in, line)) {
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        const auto eq = trimmed.find('=');
        if (eq == std::string::npos) continue;
        values_[trim(trimmed.substr(0, eq))] = trim(trimmed.substr(eq + 1));
    }
    return true;
}

bool StateStore::save() const {
    if (path_.empty()) return false;
    std::ofstream out(path_, std::ios::trunc);
    if (!out.is_open()) return false;
    out << "# RetroDisc OS state, written by the player\n";
    for (const auto& entry : values_) {
        out << entry.first << " = " << entry.second << "\n";
    }
    return out.good();
}

std::string StateStore::get(std::string_view key, std::string_view fallback) const {
    const auto it = values_.find(std::string(key));
    return it == values_.end() ? std::string(fallback) : it->second;
}

void StateStore::set(std::string_view key, std::string_view value) {
    values_[std::string(key)] = std::string(value);
}

void StateStore::remove(std::string_view key) {
    values_.erase(std::string(key));
}

}  // namespace retrodisc
