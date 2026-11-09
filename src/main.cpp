#include <cstdio>
#include <cstring>
#include <string>

#include "core/version.h"

namespace {

void printUsage() {
    std::printf(
        "RetroDisc OS %s\n"
        "\n"
        "Usage: retrodisc [options]\n"
        "\n"
        "Options:\n"
        "  --mock        Use mock CD/audio/input backends (no hardware needed)\n"
        "  --auto-disc   Simulate disc insertion at startup (requires --mock)\n"
        "  --help        Show this help\n",
        std::string(retrodisc::version()).c_str());
}

bool hasFlag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    if (hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h")) {
        printUsage();
        return 0;
    }

    const bool mock = hasFlag(argc, argv, "--mock");
    const bool autoDisc = hasFlag(argc, argv, "--auto-disc");

    std::printf("RetroDisc OS %s\n", std::string(retrodisc::version()).c_str());
    std::printf("mode: %s%s\n", mock ? "mock" : "real", autoDisc ? " (auto-disc)" : "");

    if (!mock) {
        std::fprintf(stderr, "real backends are not implemented yet (milestone M4)\n");
        return 2;
    }

    std::printf("M1 skeleton: player core not wired up yet\n");
    return 0;
}
