#pragma once

#include <string>
#include <string_view>

#include "core/state_store.h"
#include "display/theme.h"

namespace retrodisc {

// Owns the currently selected theme and persists the choice (F5 cycles it).
class ThemeManager {
   public:
    ThemeManager(std::string initialId, StateStore* store = nullptr);

    const Theme& current() const {
        return themeOrFallback(id_);
    }
    const std::string& currentId() const {
        return id_;
    }

    // Returns false for an unknown theme id and keeps the current one.
    bool set(std::string_view id);

    // Advances to the next theme and persists it.
    const Theme& cycle();

    void attachStore(StateStore* store) {
        store_ = store;
    }

    // Reads the saved theme, falling back to the given default.
    static std::string initialIdFromStore(const StateStore& store, std::string_view fallback);

   private:
    void persist();

    std::string id_;
    StateStore* store_ = nullptr;
};

}  // namespace retrodisc
