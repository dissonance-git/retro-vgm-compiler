#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Observation-only view of one command as VGMPlayer reaches it during realtime
// parsing. `payload` points into the loaded VGM image and is valid only for the
// duration of the observer callback. The observer must not mutate it or block.
struct command_event {
    std::uint64_t tick = 0;
    std::uint32_t file_offset = 0;
    std::uint8_t command = 0;
    const std::uint8_t* payload = nullptr;
    std::uint32_t payload_size = 0;
};

using command_observer = void (*)(void* user, const command_event& event) noexcept;

} // namespace gameaudio::vgm
