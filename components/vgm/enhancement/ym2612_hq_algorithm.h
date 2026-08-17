#pragma once

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Exact six-column routing representation of Nuked OPN2's eight four-operator
// algorithms. Rows are physical operators 1..4. Columns 0..4 describe the
// modulation/history inputs consumed by the OPN pipeline; column 5 identifies
// operators that feed the channel output.
//
// Keep this table as the shared topology contract for both the dependency-free
// HQ backend and the exact-state foobar/Nuked lift. Enhanced arithmetic may
// become higher precision, but the source patch must not silently acquire a
// ninth algorithm or a fifth operator.
inline constexpr std::uint8_t ym2612_hq_algorithm[4][6][8] = {
    {
        {1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,1},
    },
    {
        {0,1,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,1},
    },
    {
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {1,0,0,1,1,1,1,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,1},
    },
    {
        {0,0,1,0,0,1,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,1,0,0,0,0},
        {1,1,0,1,1,0,0,0},
        {0,0,1,0,0,0,0,0},
        {1,1,1,1,1,1,1,1},
    },
};

constexpr bool ym2612_hq_route_enabled(
    std::size_t op,
    std::size_t route,
    std::size_t algorithm) noexcept {
    return op < 4u && route < 6u && algorithm < 8u
        && ym2612_hq_algorithm[op][route][algorithm] != 0u;
}

constexpr std::size_t ym2612_hq_carrier_count(std::size_t algorithm) noexcept {
    if (algorithm >= 8u)
        return 0u;
    std::size_t count = 0;
    for (std::size_t op = 0; op < 4u; ++op)
        count += ym2612_hq_algorithm[op][5][algorithm] != 0u ? 1u : 0u;
    return count;
}

} // namespace gameaudio::vgm
