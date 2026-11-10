#pragma once

#include <string>
#include <vector>

#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include <cdio/paranoia/cdda.h>
#include <cdio/paranoia/paranoia.h>

#include "cd/cd_interface.h"

namespace retrodisc {

// CD-Text arrives as raw bytes in whatever encoding the disc used (Chinese
// discs are often GBK). The UI font is ASCII-only, so anything that is not
// printable ASCII is dropped and the track falls back to "TRACK NN".
// Revisit when the font/i18n decision is made.
std::string sanitizeCdText(const std::string& text);

// Real drive access: libcdio for the TOC (including CD-Text) and
// libcdio-paranoia for the audio sectors, so a scratchy disc gets re-read
// instead of producing clicks.
//
// libcdio types are kept out of the header's public surface as much as possible;
// everything below is an implementation detail of this backend.
class RealCDBackend : public ICDBackend {
  public:
    explicit RealCDBackend(std::string device = "/dev/sr0");
    ~RealCDBackend() override;

    RealCDBackend(const RealCDBackend&) = delete;
    RealCDBackend& operator=(const RealCDBackend&) = delete;

    bool open() override;
    void close() override;

    bool detectDisc() override;
    DiscInfo readTOC() override;
    int readFrames(int trackNumber, int startLba, int frameCount,
                   std::vector<int16_t>& out) override;
    bool eject() override;

    const std::string& device() const { return device_; }
    const std::string& lastError() const { return lastError_; }

    // Paranoia reports every re-read; useful for the SERVICE MODE page later.
    long readErrors() const { return readErrors_; }
    long sectorsRead() const { return sectorsRead_; }

  private:
    void setError(const std::string& message);
    void teardown();

    std::string device_;
    std::string lastError_;
    CdIo_t* cdio_ = nullptr;
    cdrom_drive_t* drive_ = nullptr;
    cdrom_paranoia_t* paranoia_ = nullptr;
    cdtext_t* cdtext_ = nullptr;

    DiscInfo disc_;
    lsn_t firstLsn_ = 0;      // absolute LSN of track 1, used to normalise offsets
    lsn_t nextLsn_ = -1;      // where the drive head expects to continue
    bool sequential_ = false; // true while reading forward without a seek
    bool opened_ = false;

    long readErrors_ = 0;
    long sectorsRead_ = 0;
};

}  // namespace retrodisc
