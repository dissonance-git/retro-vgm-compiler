#pragma once

#include "vgm_command_event.h"
#include "ym2612_timed_write.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Fixed-capacity, allocation-free capture of the complete YM2612 register
// timeline for both VGM chip instances. Writes retain the VGM parser's exact
// absolute tick so later synthesis can map source time to its own native clock
// without first quantizing events onto consumer output frames.
class ym2612_block_capture {
public:
    static constexpr std::size_t instance_count = 2;
    static constexpr std::size_t capacity_per_instance = 8192;

    void begin_block() noexcept {
        counts_ = {{0, 0}};
        overflow_ = {{false, false}};
        dropped_ = {{0, 0}};
    }

    void observe(const command_event& event) noexcept {
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

        const std::size_t index = counts_[instance];
        if (index >= capacity_per_instance) {
            overflow_[instance] = true;
            ++dropped_[instance];
            return;
        }

        writes_[instance][index] = ym2612_timed_write{event.tick, port, event.payload[0], event.payload[1]};
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

private:
    std::array<std::array<ym2612_timed_write, capacity_per_instance>, instance_count> writes_{};
    std::array<std::size_t, instance_count> counts_{{0, 0}};
    std::array<bool, instance_count> overflow_{{false, false}};
    std::array<std::uint64_t, instance_count> dropped_{{0, 0}};
};

} // namespace gameaudio::vgm
