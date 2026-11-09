#pragma once

#include "cd/cd_interface.h"

namespace retrodisc {

struct MockCDConfig {
    int trackCount = 12;
    int trackSeconds = 225;      // 3:45
    int lastTrackSeconds = 310;  // 5:10
    bool withCdText = true;      // odd tracks carry titles, even tracks do not
    bool failOnOpen = false;
    bool failOnToc = false;
};

// Deterministic stand-in for a real drive. Produces a known TOC and a 1 kHz
// sine wave so tests can assert on real numbers without any hardware.
class MockCDBackend : public ICDBackend {
   public:
    explicit MockCDBackend(MockCDConfig config = {});

    bool open() override;
    void close() override;
    bool detectDisc() override;
    DiscInfo readTOC() override;
    int readFrames(int trackNumber, int startLba, int frameCount,
                   std::vector<int16_t>& out) override;
    bool eject() override;

    // Simulation hooks, used by tests and by the mock runner (not part of ICDBackend).
    void insertDisc();
    void removeDisc();
    void setFailOnRead(bool fail) {
        failOnRead_ = fail;
    }
    bool isOpen() const {
        return open_;
    }
    bool hasDisc() const {
        return discPresent_;
    }
    const MockCDConfig& config() const {
        return config_;
    }

   private:
    MockCDConfig config_;
    bool open_ = false;
    bool discPresent_ = false;
    bool failOnRead_ = false;
};

}  // namespace retrodisc
