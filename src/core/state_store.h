#pragma once

#include <map>
#include <string>
#include <string_view>

namespace retrodisc {

// Tiny persistent key/value store (theme choice, resume position, ...).
// Missing files are not an error: get() simply returns the fallback.
class StateStore {
   public:
    explicit StateStore(std::string path);

    bool load();
    bool save() const;

    std::string get(std::string_view key, std::string_view fallback = {}) const;
    void set(std::string_view key, std::string_view value);
    void remove(std::string_view key);

    const std::string& path() const {
        return path_;
    }

   private:
    std::string path_;
    std::map<std::string, std::string> values_;
};

}  // namespace retrodisc
