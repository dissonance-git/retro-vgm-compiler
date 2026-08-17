#include "hes_forensic_capture.h"

namespace vgmtooling::hes {
namespace {

thread_local std::vector<apu_write_observation>* active_apu_output = nullptr;
thread_local std::int64_t active_apu_epoch = 0;
thread_local std::vector<adpcm_write_observation>* active_adpcm_output = nullptr;
thread_local std::int64_t active_adpcm_epoch = 0;

} // namespace

scoped_apu_write_capture::scoped_apu_write_capture(
    std::vector<apu_write_observation>& output) noexcept
    : previous_output_(active_apu_output),
      previous_epoch_(active_apu_epoch) {
    active_apu_output = &output;
    active_apu_epoch = 0;
}

scoped_apu_write_capture::~scoped_apu_write_capture() {
    active_apu_output = previous_output_;
    active_apu_epoch = previous_epoch_;
}

scoped_adpcm_write_capture::scoped_adpcm_write_capture(
    std::vector<adpcm_write_observation>& output) noexcept
    : previous_output_(active_adpcm_output),
      previous_epoch_(active_adpcm_epoch) {
    active_adpcm_output = &output;
    active_adpcm_epoch = 0;
}

scoped_adpcm_write_capture::~scoped_adpcm_write_capture() {
    active_adpcm_output = previous_output_;
    active_adpcm_epoch = previous_epoch_;
}

void set_apu_write_epoch(std::int64_t epoch) noexcept {
    if (active_apu_output != nullptr)
        active_apu_epoch = epoch;
}

void set_adpcm_write_epoch(std::int64_t epoch) noexcept {
    if (active_adpcm_output != nullptr)
        active_adpcm_epoch = epoch;
}

void record_apu_write(
    std::int64_t local_clock,
    int register_offset,
    int data) noexcept {
    if (active_apu_output == nullptr)
        return;
    if (register_offset < 0 || register_offset > 0x09)
        return;
    active_apu_output->push_back({
        active_apu_epoch + local_clock,
        static_cast<std::uint8_t>(register_offset),
        static_cast<std::uint8_t>(data),
    });
}

void record_adpcm_write(
    std::int64_t local_clock,
    int register_offset,
    int data) noexcept {
    if (active_adpcm_output == nullptr)
        return;
    if (register_offset < 0 || register_offset >= 0x400)
        return;
    active_adpcm_output->push_back({
        active_adpcm_epoch + local_clock,
        static_cast<std::uint16_t>(register_offset),
        static_cast<std::uint8_t>(data),
    });
}

} // namespace vgmtooling::hes
