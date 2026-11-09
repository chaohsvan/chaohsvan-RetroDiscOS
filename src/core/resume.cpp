#include "core/resume.h"

#include <cstdlib>

#include "core/state_store.h"

namespace retrodisc {
namespace {

constexpr const char* kDiscKey = "resume.disc";
constexpr const char* kTrackKey = "resume.track";
constexpr const char* kPositionKey = "resume.position";

int parseInt(const std::string& text, int fallback) {
    if (text.empty()) return fallback;
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != '\0') return fallback;
    return static_cast<int>(value);
}

}  // namespace

ResumePoint loadResume(const StateStore& store) {
    ResumePoint point;
    point.discId = store.get(kDiscKey);
    point.track = parseInt(store.get(kTrackKey), 1);
    point.positionMs = parseInt(store.get(kPositionKey), 0);
    if (point.track < 1) point.track = 1;
    if (point.positionMs < 0) point.positionMs = 0;
    return point;
}

void saveResume(StateStore& store, const ResumePoint& point) {
    if (!point.valid()) return;
    store.set(kDiscKey, point.discId);
    store.set(kTrackKey, std::to_string(point.track));
    store.set(kPositionKey, std::to_string(point.positionMs));
    store.save();
}

void clearResume(StateStore& store) {
    store.remove(kDiscKey);
    store.remove(kTrackKey);
    store.remove(kPositionKey);
    store.save();
}

}  // namespace retrodisc
