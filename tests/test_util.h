#pragma once

// Minimal test harness. No third-party dependency (see AGENTS.md section 6).
//
//   RD_TEST(name) { RD_CHECK(cond); RD_CHECK_EQ(a, b); }
//   RD_TEST_MAIN()

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace rdtest {

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) {
        registry().push_back(Case{name, std::move(fn)});
    }
};

}  // namespace rdtest

#define RD_TEST(name)                                                \
    static void name##_impl();                                       \
    static ::rdtest::Registrar name##_registrar(#name, name##_impl); \
    static void name##_impl()

#define RD_CHECK(cond)                                                                        \
    do {                                                                                      \
        if (!(cond)) {                                                                        \
            std::fprintf(stderr, "  CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++::rdtest::failures();                                                           \
        }                                                                                     \
    } while (0)

#define RD_CHECK_EQ(actual, expected)                                                          \
    do {                                                                                       \
        const auto& rd_a = (actual);                                                           \
        const auto& rd_e = (expected);                                                         \
        if (!(rd_a == rd_e)) {                                                                 \
            std::fprintf(stderr, "  CHECK_EQ failed at %s:%d: %s != %s\n", __FILE__, __LINE__, \
                         #actual, #expected);                                                  \
            ++::rdtest::failures();                                                            \
        }                                                                                      \
    } while (0)

#define RD_TEST_MAIN()                                                                     \
    int main() {                                                                           \
        int failedCases = 0;                                                               \
        for (auto& tc : ::rdtest::registry()) {                                            \
            const int before = ::rdtest::failures();                                       \
            tc.fn();                                                                       \
            const bool ok = (::rdtest::failures() == before);                              \
            if (!ok) ++failedCases;                                                        \
            std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", tc.name.c_str());               \
        }                                                                                  \
        std::printf("%zu case(s), %d failed\n", ::rdtest::registry().size(), failedCases); \
        return failedCases == 0 ? 0 : 1;                                                   \
    }
