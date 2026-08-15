#pragma once

#include "qsound_spatial_source.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class qsound_source_control_kind : std::uint8_t {
    pan = 0,
    pcm_echo_contribution = 1,
};

struct qsound_source_control_write {
    qsound_source_control_kind kind = qsound_source_control_kind::pan;
    std::uint8_t physical_slot = 0; // 0..15 PCM, 16..18 ADPCM
    std::uint16_t raw_value = 0;
};

constexpr bool qsound_decode_source_control_write(
    std::uint8_t address,
    std::uint16_t value,
    qsound_source_control_write& out) noexcept {
    // Pan controls are contiguous in physical-source order.
    if (address >= 0x80u && address <= 0x92u) {
        out.kind = qsound_source_control_kind::pan;
        out.physical_slot = static_cast<std::uint8_t>(address - 0x80u);
        out.raw_value = value;
        return true;
    }

    // Only the 16 PCM voices have the decoded per-channel echo-contribution
    // register. 0x93 is global echo feedback and must never be classified as a
    // source control merely because it follows the pan range.
    if (address >= 0xBAu && address <= 0xC9u) {
        out.kind = qsound_source_control_kind::pcm_echo_contribution;
        out.physical_slot = static_cast<std::uint8_t>(address - 0xBAu);
        out.raw_value = value;
        return true;
    }

    return false;
}

class qsound_control_state {
public:
    qsound_control_state() noexcept { reset(); }

    void reset() noexcept {
        // The recovered DL-1425 program initializes all 19 pan words to the
        // center entry at 0x120 and clears source echo state.
        pan_.fill(0x120u);
        pcm_echo_.fill(0);
    }

    bool apply(std::uint8_t address, std::uint16_t value) noexcept {
        qsound_source_control_write write;
        if (!qsound_decode_source_control_write(address, value, write))
            return false;
        return apply(write);
    }

    bool apply(const qsound_source_control_write& write) noexcept {
        if (write.kind == qsound_source_control_kind::pan) {
            if (write.physical_slot >= qsound_source_count)
                return false;
            pan_[write.physical_slot] = write.raw_value;
            return true;
        }

        if (write.kind == qsound_source_control_kind::pcm_echo_contribution) {
            if (write.physical_slot >= qsound_pcm_source_count)
                return false;
            pcm_echo_[write.physical_slot] = static_cast<std::int16_t>(write.raw_value);
            return true;
        }

        return false;
    }

    std::uint16_t pan(std::size_t physical_slot) const noexcept {
        return physical_slot < qsound_source_count ? pan_[physical_slot] : 0u;
    }

    std::int16_t pcm_echo_contribution(std::size_t pcm_slot) const noexcept {
        return pcm_slot < qsound_pcm_source_count ? pcm_echo_[pcm_slot] : 0;
    }

    qsound_spatial_source source(
        std::uint8_t instance,
        std::uint8_t physical_slot,
        std::uint32_t episode_generation = 0) const noexcept {
        if (physical_slot < qsound_pcm_source_count) {
            return make_qsound_spatial_source(
                qsound_source_kind::pcm,
                instance,
                physical_slot,
                episode_generation,
                pan_[physical_slot],
                pcm_echo_[physical_slot]);
        }

        const std::uint8_t adpcm_slot = static_cast<std::uint8_t>(physical_slot - qsound_pcm_source_count);
        return make_qsound_spatial_source(
            qsound_source_kind::adpcm,
            instance,
            adpcm_slot,
            episode_generation,
            physical_slot < qsound_source_count ? pan_[physical_slot] : 0u);
    }

private:
    std::array<std::uint16_t, qsound_source_count> pan_{};
    std::array<std::int16_t, qsound_pcm_source_count> pcm_echo_{};
};

} // namespace gameaudio::vgm
