#include "hes_forensic_capture.h"

#include <cstddef>

namespace vgmtooling::hes {
namespace {

thread_local std::vector<apu_write_observation>* active_capture = nullptr;

} // namespace

scoped_apu_write_capture::scoped_apu_write_capture(
    std::vector<apu_write_observation>& output) noexcept
    : previous_(active_capture) {
    active_capture = &output;
}

scoped_apu_write_capture::~scoped_apu_write_capture() {
    active_capture = previous_;
}

void record_apu_write(std::int64_t clock, int register_offset, int data) noexcept {
    if (active_capture == nullptr)
        return;
    if (register_offset < 0 || register_offset > 0x09)
        return;
    active_capture->push_back({
        clock,
        static_cast<std::uint8_t>(register_offset),
        static_cast<std::uint8_t>(data),
    });
}

} // namespace vgmtooling::hes
