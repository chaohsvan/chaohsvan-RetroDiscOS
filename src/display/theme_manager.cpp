#include "display/theme_manager.h"

#include <utility>

namespace retrodisc {
namespace {
constexpr const char* kThemeKey = "theme";
}  // namespace

ThemeManager::ThemeManager(std::string initialId, StateStore* store)
    : id_(std::move(initialId)), store_(store) {
    if (themeById(id_) == nullptr) id_ = defaultTheme().id;
}

bool ThemeManager::set(std::string_view id) {
    if (themeById(id) == nullptr) return false;
    id_ = std::string(id);
    persist();
    return true;
}

const Theme& ThemeManager::cycle() {
    id_ = nextThemeId(id_);
    persist();
    return current();
}

void ThemeManager::persist() {
    if (store_ == nullptr) return;
    store_->set(kThemeKey, id_);
    store_->save();
}

std::string ThemeManager::initialIdFromStore(const StateStore& store, std::string_view fallback) {
    const std::string saved = store.get(kThemeKey, fallback);
    return themeById(saved) != nullptr ? saved : std::string(fallback);
}

}  // namespace retrodisc
