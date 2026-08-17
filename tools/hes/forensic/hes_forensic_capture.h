#pragma once

#include <cstdint>
#include <vector>

namespace vgmtooling::hes {

struct apu_write_observation {
    std::int64_t clock = 0;
    std::uint8_t register_offset = 0;
    std::uint8_t data = 0;
};

struct adpcm_write_observation {
    std::int64_t clock = 0;
    std::uint16_t register_offset = 0;
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
    std::vector<apu_write_observation>* previous_output_ = nullptr;
    std::int64_t previous_epoch_ = 0;
};

class scoped_adpcm_write_capture {
public:
    explicit scoped_adpcm_write_capture(std::vector<adpcm_write_observation>& output) noexcept;
    ~scoped_adpcm_write_capture();

    scoped_adpcm_write_capture(const scoped_adpcm_write_capture&) = delete;
    scoped_adpcm_write_capture& operator=(const scoped_adpcm_write_capture&) = delete;

private:
    std::vector<adpcm_write_observation>* previous_output_ = nullptr;
    std::int64_t previous_epoch_ = 0;
};

// Hes_Cpu::time() is local to the current run_clocks() frame because libgme
// rebases CPU time after every frame. The forensic driver owns the exact frame
// duration and sets both epochs before execution, so recorded clocks remain
// absolute without guessing from timestamp rollbacks.
void set_apu_write_epoch(std::int64_t epoch) noexcept;
void set_adpcm_write_epoch(std::int64_t epoch) noexcept;
void record_apu_write(std::int64_t local_clock, int register_offset, int data) noexcept;
void record_adpcm_write(std::int64_t local_clock, int register_offset, int data) noexcept;

} // namespace vgmtooling::hes
