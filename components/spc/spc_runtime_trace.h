#pragma once

#include "spc_runtime_capture.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace gameaudio::spc {

// Lossless offline facts observed after an exact SPC snapshot. The snapshot
// remains the sole initial-state authority. One RAM-write record corresponds to
// one emulated source mutation boundary and therefore one generation serial.
struct spc_runtime_trace_ram_write {
    std::uint64_t serial = 0;
    std::uint16_t address = 0;
    std::vector<std::uint8_t> bytes;
};

// A drained fixed-capacity DSP capture window. Overflow is preserved explicitly
// so missing observations become a semantic continuity break during replay.
struct spc_runtime_trace_window {
    std::vector<spc_runtime_capture_record> records;
    bool overflowed = false;
    std::uint64_t dropped = 0;
    std::optional<spc_runtime_capture_record> first_dropped{};
    std::uint64_t next_trace_index = 0;
};

struct spc_runtime_trace {
    std::vector<spc_runtime_trace_ram_write> ram_writes;
    std::vector<spc_runtime_trace_window> windows;
};

} // namespace gameaudio::spc
