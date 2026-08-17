#pragma once

#include <cstdint>
#include <vector>

namespace vgmtooling::hes {

struct apu_write_observation {
    std::int64_t clock = 0;
    std::uint8_t register_offset = 0;
    std::uint8_t data = 0;
};

// Capture is thread-local because libgme may be used by unrelated playback code
// in the same process. Nesting is supported so a bounded forensic scope can be
// composed without stealing an outer observer.
class scoped_apu_write_capture {
public:
    explicit scoped_apu_write_capture(std::vector<apu_write_observation>& output) noexcept;
    ~scoped_apu_write_capture();

    scoped_apu_write_capture(const scoped_apu_write_capture&) = delete;
    scoped_apu_write_capture& operator=(const scoped_apu_write_capture&) = delete;

private:
    std::vector<apu_write_observation>* previous_ = nullptr;
};

void record_apu_write(std::int64_t clock, int register_offset, int data) noexcept;

} // namespace vgmtooling::hes
