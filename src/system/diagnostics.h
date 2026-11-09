#pragma once

#include <string>
#include <vector>

namespace retrodisc {

struct DiagnosticItem {
    std::string label;
    std::string value;
    bool available = false;
};

// SERVICE MODE data source (PRD 2.8). Everything comes from sysfs/proc and
// degrades to "N/A" when a device or file is missing — never throws, never exits.
class Diagnostics {
   public:
    Diagnostics();
    // root is prefixed to every path; tests pass a fake tree.
    explicit Diagnostics(std::string root);

    std::vector<DiagnosticItem> collect() const;

    static constexpr int kItemCount = 10;

   private:
    std::string path(const std::string& relative) const;

    std::string root_;
};

}  // namespace retrodisc
