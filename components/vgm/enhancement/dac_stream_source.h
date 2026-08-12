#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class dac_stream_source_event_kind : std::uint8_t {
    setup = 0,
    set_data = 1,
    set_frequency = 2,
    start = 3,
    stop = 4,
};

// Source-level view of one VGM DAC stream after libvgm has resolved its bank,
// destination and playback parameters. This is deliberately chip-neutral; a
// renderer opts in only when it understands the destination semantics.
struct dac_stream_source_event {
    dac_stream_source_event_kind kind = dac_stream_source_event_kind::setup;
    std::uint64_t sample = 0;
    std::uint8_t stream_id = 0;
    std::uint8_t chip_type = 0xFF;
    std::uint8_t chip_id = 0;
    std::uint16_t chip_command = 0;
    std::uint8_t bank_id = 0xFF;
    std::uint8_t step_size = 1;
    std::uint8_t step_base = 0;
    std::uint8_t play_mode = 0;
    std::uint32_t frequency = 0;
    std::uint32_t start_offset = 0;
    std::uint32_t length = 0;
    const std::uint8_t* data = nullptr;
    std::size_t data_length = 0;
};

} // namespace gameaudio::vgm
