#pragma once

#include "vgm_command_event.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct ym2612_timed_write {
    std::size_t sample_offset = 0;
    std::uint8_t port = 0;
    std::uint8_t reg = 0;
    std::uint8_t data = 0;
};

// Fixed-capacity, allocation-free capture of the complete YM2612 register
// timeline for both VGM chip instances. This is the input contract for a future
// high-resolution FM renderer; it preserves source automation rather than
// snapshotting one patch per 10 ms output block.
class ym2612_block_capture {
public:
    static constexpr std::size_t instance_count = 2;
    static constexpr std::size_t capacity_per_instance = 8192;

    void begin_block(std::uint64_t start_sample) noexcept {
        block_start_sample_ = start_sample;
        counts_ = {{0, 0}};
        overflow_ = {{false, false}};
        dropped_ = {{0, 0}};
    }

    void observe(const command_event& event, std::uint64_t absolute_sample) noexcept {
        if (event.kind != command_event_kind::command || event.payload == nullptr || event.payload_size < 2)
            return;

        std::size_t instance = 0;
        std::uint8_t port = 0;
        switch (event.command) {
        case 0x52:
            instance = 0;
            port = 0;
            break;
        case 0x53:
            instance = 0;
            port = 1;
            break;
        case 0xA2:
            instance = 1;
            port = 0;
            break;
        case 0xA3:
            instance = 1;
            port = 1;
            break;
        default:
            return;
        }

        std::uint64_t delta = 0;
        if (absolute_sample > block_start_sample_)
            delta = absolute_sample - block_start_sample_;
        const std::uint64_t max_size = static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max());
        const std::size_t offset = static_cast<std::size_t>(delta > max_size ? max_size : delta);

        const std::size_t index = counts_[instance];
        if (index >= capacity_per_instance) {
            overflow_[instance] = true;
            ++dropped_[instance];
            return;
        }

        writes_[instance][index] = ym2612_timed_write{offset, port, event.payload[0], event.payload[1]};
        counts_[instance] = index + 1;
    }

    const ym2612_timed_write* writes(std::size_t instance) const noexcept {
        return writes_[instance & 1u].data();
    }

    std::size_t count(std::size_t instance) const noexcept {
        return counts_[instance & 1u];
    }

    bool overflowed(std::size_t instance) const noexcept {
        return overflow_[instance & 1u];
    }

    std::uint64_t dropped(std::size_t instance) const noexcept {
        return dropped_[instance & 1u];
    }

    std::uint64_t block_start_sample() const noexcept { return block_start_sample_; }

private:
    std::array<std::array<ym2612_timed_write, capacity_per_instance>, instance_count> writes_{};
    std::array<std::size_t, instance_count> counts_{{0, 0}};
    std::array<bool, instance_count> overflow_{{false, false}};
    std::array<std::uint64_t, instance_count> dropped_{{0, 0}};
    std::uint64_t block_start_sample_ = 0;
};

} // namespace gameaudio::vgm
