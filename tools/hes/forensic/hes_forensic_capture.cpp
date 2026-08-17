#include "hes_forensic_capture.h"

namespace vgmtooling::hes {
namespace {

thread_local std::vector<apu_write_observation>* active_output = nullptr;
thread_local std::int64_t active_epoch = 0;

} // namespace

scoped_apu_write_capture::scoped_apu_write_capture(
    std::vector<apu_write_observation>& output) noexcept
    : previous_output_(active_output),
      previous_epoch_(active_epoch) {
    active_output = &output;
    active_epoch = 0;
}

scoped_apu_write_capture::~scoped_apu_write_capture() {
    active_output = previous_output_;
    active_epoch = previous_epoch_;
}

void set_apu_write_epoch(std::int64_t epoch) noexcept {
    if (active_output != nullptr)
        active_epoch = epoch;
}

void record_apu_write(
    std::int64_t local_clock,
    int register_offset,
    int data) noexcept {
    if (active_output == nullptr)
        return;
    if (register_offset < 0 || register_offset > 0x09)
        return;
    active_output->push_back({
        active_epoch + local_clock,
        static_cast<std::uint8_t>(register_offset),
        static_cast<std::uint8_t>(data),
    });
}

} // namespace vgmtooling::hes
