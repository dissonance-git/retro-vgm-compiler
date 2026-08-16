#pragma once

#include "spc_runtime_trace.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Minimal producer-facing boundary for an instrumented SPC execution core.
// Implementations report exact device facts only. They do not construct musical
// graphs, resolve persistent parts, read tags, or know anything about attribution.
class spc_runtime_instrumentation_sink {
public:
    virtual ~spc_runtime_instrumentation_sink() = default;

    virtual std::uint64_t observe_apuram_write(
        spc_runtime_ram_write_origin origin,
        std::int64_t tick,
        std::uint64_t tick_rate,
        std::uint16_t address,
        const std::uint8_t* bytes,
        std::size_t byte_count) = 0;

    virtual void observe_voice_event(spc_runtime_capture_record record) = 0;
};

} // namespace gameaudio::spc
