#pragma once

#include "sn76489_enhanced.h"
#include "vgm_command_event.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

// Fixed-capacity collector for the two SN76489-family instances representable
// by VGM. It is designed for the realtime observer callback: no allocation,
// locking, parsing, or audio processing occurs while libvgm is rendering.
class psg_block_capture {
public:
    static constexpr std::size_t instance_count = 2;
    static constexpr std::size_t capacity_per_instance = 2048;

    void begin_block(std::uint64_t start_sample) noexcept {
        block_start_sample_ = start_sample;
        counts_ = {{0, 0}};
        overflow_ = {{false, false}};
        dropped_ = {{0, 0}};
    }

    void observe(const command_event& event, std::uint64_t absolute_sample) noexcept {
        if (event.kind != command_event_kind::command || event.payload == nullptr || event.payload_size < 1)
            return;

        std::size_t instance = 0;
        sn76489_write_kind kind = sn76489_write_kind::register_write;

        switch (event.command) {
        case 0x50:
            instance = 0;
            break;
        case 0x30:
            instance = 1;
            break;
        case 0x4F:
            instance = 0;
            kind = sn76489_write_kind::stereo_mask;
            break;
        default:
            return;
        }

        std::uint64_t delta = 0;
        if (absolute_sample > block_start_sample_)
            delta = absolute_sample - block_start_sample_;
        const std::uint64_t size_max = static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max());
        const std::size_t offset = static_cast<std::size_t>(delta > size_max ? size_max : delta);

        const std::size_t index = counts_[instance];
        if (index >= capacity_per_instance) {
            overflow_[instance] = true;
            ++dropped_[instance];
            return;
        }

        writes_[instance][index] = sn76489_timed_write{offset, kind, event.payload[0]};
        counts_[instance] = index + 1;
    }

    const sn76489_timed_write* writes(std::size_t instance) const noexcept {
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
    std::array<std::array<sn76489_timed_write, capacity_per_instance>, instance_count> writes_{};
    std::array<std::size_t, instance_count> counts_{{0, 0}};
    std::array<bool, instance_count> overflow_{{false, false}};
    std::array<std::uint64_t, instance_count> dropped_{{0, 0}};
    std::uint64_t block_start_sample_ = 0;
};

} // namespace gameaudio::vgm
