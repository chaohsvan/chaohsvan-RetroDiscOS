#include "core/version.h"

#ifndef RETRODISC_VERSION
#define RETRODISC_VERSION "0.0.0-unknown"
#endif

namespace retrodisc {

std::string_view version() {
    return RETRODISC_VERSION;
}

}  // namespace retrodisc
