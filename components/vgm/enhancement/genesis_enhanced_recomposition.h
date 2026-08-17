#pragma once

#include "source_family_recomposition.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Canonical Genesis source order used by the source-aware foobar/libvgm seam.
// These are implementation sources, not persistent musical parts.
enum class genesis_recomposition_source : std::uint8_t {
    ym2612_fm1 = 0,
    ym2612_fm2,
    ym2612_fm3,
    ym2612_fm4,
    ym2612_fm5,
    ym2612_fm6,
    ym2612_dac,
    sn76489_tone0,
    sn76489_tone1,
    sn76489_tone2,
    sn76489_noise,
    count,
};

constexpr std::size_t genesis_recomposition_source_count =
    static_cast<std::size_t>(genesis_recomposition_source::count);

enum class genesis_recomposition_family : std::uint8_t {
    ym2612_fm = 0,
    ym2612_dac,
    sn76489_psg,
    count,
};

constexpr std::size_t genesis_recomposition_family_count =
    static_cast<std::size_t>(genesis_recomposition_family::count);

// Keep the established Genesis API names while delegating the shared exact-source
// transaction law to the chip-neutral VGM source-family engine.
using genesis_enhanced_recomposition_error = source_family_recomposition_error;
using genesis_stereo_source_view = source_family_stereo_view;
using genesis_source_replacement_view = source_family_replacement_view;
using genesis_recomposition_family_status = source_family_recomposition_status;

template <std::size_t MaxFrames = 4096>
class genesis_enhanced_recomposition_storage {
public:
    using engine_type = source_family_recomposition_storage<
        genesis_recomposition_source_count,
        genesis_recomposition_family_count,
        MaxFrames>;
    using source_array = typename engine_type::source_array;

    void reset() noexcept { engine_.reset(); }

    // Strict scientific/control transaction. Any bad requested source rejects
    // the entire candidate block and restores the protected reference exactly.
    bool build(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count,
        const source_array& sources) noexcept
    {
        return engine_.build(reference_left, reference_right, frame_count, sources);
    }

    // Product-facing Genesis policy. FM, DAC, and PSG remain separate evidence
    // families even though the transaction machinery is now shared with other
    // VGM devices. A failed family stays reference while proven families apply.
    bool build_independent_families(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count,
        const source_array& sources) noexcept
    {
        return engine_.build_independent_families(
            reference_left,
            reference_right,
            frame_count,
            sources,
            source_families_);
    }

    bool valid() const noexcept { return engine_.valid(); }
    bool used_replacement() const noexcept { return engine_.used_replacement(); }
    std::size_t frame_count() const noexcept { return engine_.frame_count(); }
    const float* left() const noexcept { return engine_.left(); }
    const float* right() const noexcept { return engine_.right(); }
    genesis_enhanced_recomposition_error last_error() const noexcept {
        return engine_.last_error();
    }

    genesis_recomposition_family_status family_status(
        genesis_recomposition_family family) const noexcept
    {
        return engine_.family_status(static_cast<std::size_t>(family));
    }

    bool had_family_failure() const noexcept { return engine_.had_family_failure(); }

private:
    inline static constexpr typename engine_type::family_map source_families_{
        0, 0, 0, 0, 0, 0,
        1,
        2, 2, 2, 2,
    };

    engine_type engine_{};
};

} // namespace gameaudio::vgm
