#pragma once

#include "vgm_command_event.h"
#include "ym2612_dac_enhanced.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

// Fixed-capacity realtime capture for both YM2612 instances. A 10 ms VGM block
// normally contains only hundreds of DAC writes even at the format's 44.1 kHz
// tick rate; the larger capacity leaves substantial headroom without allocating
// on the decode path.
class ym2612_dac_block_capture {
public:
    static constexpr std::size_t instance_count = 2;
    static constexpr std::size_t capacity_per_instance = 2048;

    void begin_block(std::uint64_t start_sample) noexcept {
        block_start_sample_ = start_sample;
        counts_ = {{0, 0}};
        overflow_ = {{false, false}};
        dropped_ = {{0, 0}};
        pan_changed_ = {{false, false}};
    }

    void observe(const command_event& event, std::uint64_t absolute_sample) noexcept {
        std::size_t instance = 0;
        ym2612_dac_event_kind kind = ym2612_dac_event_kind::data;
        std::uint8_t value = 0;

        if (event.kind == command_event_kind::ym2612_dac) {
            if (event.payload == nullptr || event.payload_size < 1)
                return;
            instance = 0;
            value = event.payload[0];
        } else if (event.kind == command_event_kind::command) {
            if (event.payload == nullptr || event.payload_size < 2)
                return;

            // Channel 6 pan lives at B6 on YM2612 port 1. The first audible DAC
            // rung deliberately admits only blocks whose pan is stable from the
            // captured block origin. This records the contrary evidence rather
            // than guessing stereo from the distorted reference DAC lane.
            if (event.command == 0x53 || event.command == 0xA3) {
                instance = event.command == 0xA3 ? 1 : 0;
                if (event.payload[0] == 0xB6)
                    pan_changed_[instance] = true;
                return;
            }

            if (event.command == 0x52 || event.command == 0xA2)
                instance = event.command == 0xA2 ? 1 : 0;
            else
                return;

            const std::uint8_t reg = event.payload[0];
            if (reg == 0x2A) {
                kind = ym2612_dac_event_kind::data;
                value = event.payload[1];
            } else if (reg == 0x2B) {
                kind = ym2612_dac_event_kind::enable;
                value = event.payload[1];
            } else {
                return;
            }
        } else {
            return;
        }

        std::uint64_t delta = 0;
        if (absolute_sample > block_start_sample_)
            delta = absolute_sample - block_start_sample_;
        const auto max_size = static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max());
        const std::size_t offset = static_cast<std::size_t>(delta > max_size ? max_size : delta);

        const std::size_t index = counts_[instance];
        if (index >= capacity_per_instance) {
            overflow_[instance] = true;
            ++dropped_[instance];
            return;
        }

        events_[instance][index] = ym2612_dac_timed_event{offset, kind, value};
        counts_[instance] = index + 1;
    }

    const ym2612_dac_timed_event* events(std::size_t instance) const noexcept {
        return events_[instance & 1u].data();
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

    bool pan_changed(std::size_t instance) const noexcept {
        return pan_changed_[instance & 1u];
    }

    std::uint64_t block_start_sample() const noexcept { return block_start_sample_; }

private:
    std::array<std::array<ym2612_dac_timed_event, capacity_per_instance>, instance_count> events_{};
    std::array<std::size_t, instance_count> counts_{{0, 0}};
    std::array<bool, instance_count> overflow_{{false, false}};
    std::array<std::uint64_t, instance_count> dropped_{{0, 0}};
    std::array<bool, instance_count> pan_changed_{{false, false}};
    std::uint64_t block_start_sample_ = 0;
};

} // namespace gameaudio::vgm
