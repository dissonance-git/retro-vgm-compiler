#pragma once

#include <cstdint>

namespace gameaudio::vgm {

// Register-layout facts that survive across at least the OPN and OPM Yamaha
// four-operator FM families. The surrounding addresses, pitch encoding, key
// encoding, clocks and synthesis details remain family-specific.
struct yamaha_algorithm_feedback {
    std::uint8_t algorithm = 0;
    std::uint8_t feedback = 0;
};

constexpr yamaha_algorithm_feedback decode_yamaha_algorithm_feedback(
    const std::uint8_t data) noexcept {
    return yamaha_algorithm_feedback{
        static_cast<std::uint8_t>(data & 0x07u),
        static_cast<std::uint8_t>((data >> 3) & 0x07u),
    };
}

// Yamaha's physical register slot order for these four-operator families is
// 1,3,2,4. Map the two-bit register slot to the logical operator order 1,2,3,4.
constexpr std::uint8_t yamaha_four_op_logical_operator(
    const std::uint8_t register_slot) noexcept {
    constexpr std::uint8_t slot_to_operator[4] = {0, 2, 1, 3};
    return slot_to_operator[register_slot & 0x03u];
}

} // namespace gameaudio::vgm
