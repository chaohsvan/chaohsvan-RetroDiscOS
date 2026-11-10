#include "cd/real_cd.h"

#include <fcntl.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <utility>

namespace retrodisc {
namespace {

std::string cdtextField(cdtext_t* cdtext, cdtext_field_t field, track_t track) {
    if (cdtext == nullptr) return {};
    const char* value = cdtext_get_const(cdtext, field, track);
    return value != nullptr ? sanitizeCdText(value) : std::string();
}

}  // namespace

std::string sanitizeCdText(const std::string& text) {
    if (text.empty()) return {};
    for (unsigned char c : text) {
        if (c < 0x20 || c > 0x7E) return {};  // non-ASCII or control byte
    }
    return text;
}

RealCDBackend::RealCDBackend(std::string device) : device_(std::move(device)) {}

RealCDBackend::~RealCDBackend() { close(); }

void RealCDBackend::setError(const std::string& message) { lastError_ = message; }

void RealCDBackend::teardown() {
    if (paranoia_ != nullptr) {
        cdio_paranoia_free(paranoia_);
        paranoia_ = nullptr;
    }
    if (drive_ != nullptr) {
        // cdio_cddap_identify_cdio() hands the CdIo handle to the drive object,
        // and cdio_cddap_close() frees it. Freeing cdio_ as well is a double free.
        cdio_cddap_close(drive_);
        drive_ = nullptr;
        cdio_ = nullptr;
    } else if (cdio_ != nullptr) {
        cdio_free(cdio_);
        cdio_ = nullptr;
    }
    cdtext_ = nullptr;
}

bool RealCDBackend::open() {
    if (cdio_ != nullptr) close();

    // Reject anything that is not a block device before handing the path to
    // libcdio: it would otherwise try image drivers and leak on failure.
    struct stat info {};
    if (::stat(device_.c_str(), &info) != 0) {
        setError(device_ + ": " + std::strerror(errno));
        return false;
    }
    if (!S_ISBLK(info.st_mode)) {
        setError(device_ + " is not a block device (expected something like /dev/sr0)");
        return false;
    }

    cdio_ = cdio_open(device_.c_str(), DRIVER_UNKNOWN);
    if (cdio_ == nullptr) {
        setError("cdio_open(" + device_ + ") failed: " + std::strerror(errno));
        return false;
    }

    const discmode_t mode = cdio_get_discmode(cdio_);
    if (mode != CDIO_DISC_MODE_CD_DA && mode != CDIO_DISC_MODE_CD_MIXED) {
        setError(device_ + " does not hold an audio CD");
        teardown();
        return false;
    }

    // libcdio-paranoia wants its own drive object; reuse the CdIo handle so the
    // TOC and the audio path see exactly the same disc.
    drive_ = cdio_cddap_identify_cdio(cdio_, CDDA_MESSAGE_FORGETIT, nullptr);
    if (drive_ == nullptr) {
        setError("cdio_cddap_identify_cdio failed");
        teardown();
        return false;
    }
    if (cdio_cddap_open(drive_) != 0) {
        setError("cdio_cddap_open failed");
        teardown();
        return false;
    }
    paranoia_ = cdio_paranoia_init(drive_);
    if (paranoia_ == nullptr) {
        setError("cdio_paranoia_init failed");
        teardown();
        return false;
    }
    // Full verification: re-read suspicious sectors instead of emitting clicks.
    cdio_paranoia_modeset(paranoia_, PARANOIA_MODE_FULL);

    cdtext_ = cdio_get_cdtext(cdio_);
    opened_ = true;
    lastError_.clear();
    return true;
}

void RealCDBackend::close() {
    teardown();
    opened_ = false;
    disc_ = DiscInfo{};
    firstLsn_ = 0;
}

bool RealCDBackend::detectDisc() {
    if (!opened_ || cdio_ == nullptr) return false;
    const discmode_t mode = cdio_get_discmode(cdio_);
    return mode == CDIO_DISC_MODE_CD_DA || mode == CDIO_DISC_MODE_CD_MIXED;
}

DiscInfo RealCDBackend::readTOC() {
    DiscInfo info;
    if (!detectDisc()) {
        setError("no audio disc");
        return info;
    }

    const track_t first = cdio_get_first_track_num(cdio_);
    const track_t last = cdio_get_num_tracks(cdio_);
    if (first == CDIO_INVALID_TRACK || last == CDIO_INVALID_TRACK || last < first) {
        setError("cannot enumerate tracks");
        return info;
    }

    firstLsn_ = cdio_get_track_lsn(cdio_, first);
    if (firstLsn_ == CDIO_INVALID_LSN) {
        setError("cannot read the first track position");
        return info;
    }

    info.valid = true;
    info.trackCount = static_cast<int>(last - first + 1);

    lsn_t endLsn = CDIO_INVALID_LSN;
    for (track_t track = first; track <= last; ++track) {
        const lsn_t lsn = cdio_get_track_lsn(cdio_, track);
        if (lsn == CDIO_INVALID_LSN) {
            setError("cannot read track " + std::to_string(track));
            return DiscInfo{};
        }

        TrackInfo entry;
        entry.number = static_cast<int>(track - first + 1);
        entry.startLba = static_cast<int>(lsn - firstLsn_);
        entry.title = cdtextField(cdtext_, CDTEXT_FIELD_TITLE, track);

        // The last track ends at the lead-out; the others end where the next begins.
        if (track < last) {
            const lsn_t next = cdio_get_track_lsn(cdio_, track + 1);
            entry.lengthSectors = next == CDIO_INVALID_LSN ? 0 : static_cast<int>(next - lsn);
        } else {
            endLsn = cdio_get_track_lsn(cdio_, CDIO_CDROM_LEADOUT_TRACK);
            if (endLsn == CDIO_INVALID_LSN) endLsn = cdio_get_disc_last_lsn(cdio_);
            entry.lengthSectors = endLsn == CDIO_INVALID_LSN ? 0 : static_cast<int>(endLsn - lsn);
        }

        if (entry.lengthSectors <= 0) {
            setError("track " + std::to_string(entry.number) + " has no length");
            return DiscInfo{};
        }

        info.totalSectors += entry.lengthSectors;
        info.tracks.push_back(std::move(entry));
    }

    // Stable per-disc id: same disc, same id; different disc, different id.
    info.discId = "CD-" + std::to_string(info.trackCount) + "-" +
                  std::to_string(static_cast<long long>(firstLsn_)) + "-" +
                  std::to_string(info.totalSectors);

    disc_ = info;
    return info;
}

int RealCDBackend::readFrames(int trackNumber, int startLba, int frameCount,
                              std::vector<int16_t>& out) {
    out.clear();
    if (!opened_ || paranoia_ == nullptr) return -1;
    if (!disc_.valid) {
        if (!readTOC().valid) return -1;
    }
    if (frameCount <= 0) return -1;

    const TrackInfo* track = disc_.track(trackNumber);
    if (track == nullptr) return -1;
    if (startLba < 0 || startLba >= track->lengthSectors) return -1;

    const int remaining = track->lengthSectors - startLba;
    const int framesWanted = std::min(frameCount, remaining * kFramesPerSector);
    const int sectors = (framesWanted + kFramesPerSector - 1) / kFramesPerSector;

    const lsn_t absolute = firstLsn_ + track->startLba + startLba;
    if (cdio_paranoia_seek(paranoia_, static_cast<int32_t>(absolute), SEEK_SET) < 0) {
        setError("paranoia seek failed");
        return -1;
    }

    out.reserve(static_cast<std::size_t>(sectors) * kFramesPerSector * kChannels);
    for (int i = 0; i < sectors; ++i) {
        int16_t* data = cdio_paranoia_read(paranoia_, nullptr);
        if (data == nullptr) {
            ++readErrors_;
            setError("paranoia read failed at sector " + std::to_string(absolute + i));
            break;
        }
        out.insert(out.end(), data, data + kFramesPerSector * kChannels);
        ++sectorsRead_;
    }

    if (out.empty()) return -1;
    const int frames = static_cast<int>(out.size() / kChannels);
    return std::min(frames, framesWanted);
}

bool RealCDBackend::eject() {
    // Release the drive first, otherwise the kernel may refuse the ioctl.
    const bool wasOpen = opened_;
    if (wasOpen) {
        teardown();
        opened_ = false;
    }

    const int fd = ::open(device_.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        setError("open " + device_ + ": " + std::strerror(errno));
        return false;
    }
    const int rc = ::ioctl(fd, CDROMEJECT, 0);
    ::close(fd);
    if (rc < 0) {
        setError(std::string("CDROMEJECT: ") + std::strerror(errno));
        return false;
    }
    return true;
}

}  // namespace retrodisc
