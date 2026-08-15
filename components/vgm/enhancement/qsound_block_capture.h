#pragma once

#include "qsound_control_state.h"
#include "vgm_command_event.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

struct qsound_timed_source_control {
    std::size_t sample_offset = 0;
    qsound_source_control_write write{};
};

// Realtime-safe collector for source-facing QSound C4 writes. It intentionally
// ignores global echo/FIR/delay/output state because those belong to the
// historical QSound renderer, not to one causal source.
class qsound_block_capture {
public:
    static constexpr std::size_t capacity = 4096;

    void begin_block(std::uint64_t start_sample) noexcept {
        block_start_sample_ = start_sample;
        count_ = 0;
        overflow_ = false;
        dropped_ = 0;
    }

    void observe(const command_event& event, std::uint64_t absolute_sample) noexcept {
        if (event.kind != command_event_kind::command || event.command != 0xC4u ||
            event.payload == nullptr || event.payload_size < 3u)
            return;

        const std::uint16_t value = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(event.payload[0]) << 8u) |
            static_cast<std::uint16_t>(event.payload[1]));
        const std::uint8_t address = event.payload[2];

        qsound_source_control_write write;
        if (!qsound_decode_source_control_write(address, value, write))
            return;

        std::uint64_t delta = 0;
        if (absolute_sample > block_start_sample_)
            delta = absolute_sample - block_start_sample_;
        const std::uint64_t size_max = static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max());
        const std::size_t offset = static_cast<std::size_t>(delta > size_max ? size_max : delta);

        if (count_ >= capacity) {
            overflow_ = true;
            ++dropped_;
            return;
        }

        controls_[count_++] = qsound_timed_source_control{offset, write};
    }

    const qsound_timed_source_control* controls() const noexcept { return controls_.data(); }
    std::size_t count() const noexcept { return count_; }
    bool overflowed() const noexcept { return overflow_; }
    std::uint64_t dropped() const noexcept { return dropped_; }
    std::uint64_t block_start_sample() const noexcept { return block_start_sample_; }

private:
    std::array<qsound_timed_source_control, capacity> controls_{};
    std::size_t count_ = 0;
    bool overflow_ = false;
    std::uint64_t dropped_ = 0;
    std::uint64_t block_start_sample_ = 0;
};

} // namespace gameaudio::vgm
