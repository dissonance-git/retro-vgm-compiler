#pragma once

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

// SMPS sequence bytes preserve a discrete chromatic coordinate, not a unique
// written spelling. The historical SMPS2ASM equates assign nRst=$80, nC0=$81,
// and then advance one byte per chromatic step; enharmonic spellings such as
// C#/Db are aliases of the same compiled byte.
//
// This representation deliberately stops before device tuning/frequency,
// detune/modulation, performed/heard pitch, note spelling, key, or harmony.
struct smps_programmed_pitch_coordinate {
    int chromatic_steps_from_c0 = 0;
};

inline constexpr bool smps_is_rest_token(std::uint8_t token) noexcept {
    return token == 0x80u;
}

inline constexpr bool smps_is_note_token(std::uint8_t token) noexcept {
    // 0x80 is rest and 0xE0 begins the coordinate-flag range in the S&K
    // driver. 0x81..0xDF are therefore the compiled note-token domain.
    return token >= 0x81u && token < 0xE0u;
}

// Track transposition is an explicitly separate semitone displacement in SMPS.
// Keep the result as a signed coordinate: a malformed/out-of-table result is a
// later driver-validity question, not a reason to wrap the musical coordinate.
inline std::optional<smps_programmed_pitch_coordinate> smps_programmed_pitch_from_token(
    std::uint8_t note_token,
    std::int8_t semitone_transposition = 0) noexcept {
    if (!smps_is_note_token(note_token))
        return std::nullopt;

    return smps_programmed_pitch_coordinate{
        static_cast<int>(note_token - 0x81u) +
        static_cast<int>(semitone_transposition),
    };
}

} // namespace gameaudio::vgm
