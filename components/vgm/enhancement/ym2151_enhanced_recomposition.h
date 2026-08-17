#pragma once

#include "source_family_recomposition.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// YM2151/OPM exposes eight FM channel source identities. Each source represents
// one complete four-operator channel after its authoritative OPM algorithm,
// operator ratios/detune/envelopes, feedback, LFO/PM/AM state and authored
// stereo route have been realized. Operators are not independent replacement
// sources because their network topology is part of the channel's timbre.
enum class ym2151_recomposition_source : std::uint8_t {
    fm1 = 0,
    fm2,
    fm3,
    fm4,
    fm5,
    fm6,
    fm7,
    fm8,
    count,
};

constexpr std::size_t ym2151_recomposition_source_count =
    static_cast<std::size_t>(ym2151_recomposition_source::count);

enum class ym2151_recomposition_family : std::uint8_t {
    fm = 0,
    count,
};

constexpr std::size_t ym2151_recomposition_family_count =
    static_cast<std::size_t>(ym2151_recomposition_family::count);

using ym2151_enhanced_recomposition_error = source_family_recomposition_error;
using ym2151_stereo_source_view = source_family_stereo_view;
using ym2151_source_replacement_view = source_family_replacement_view;
using ym2151_recomposition_family_status = source_family_recomposition_status;

// First non-Genesis client of the generic VGM source-family transaction law.
// This layer deliberately does not synthesize OPM itself. It admits/rejects an
// independently validated higher-quality OPM realization while guaranteeing
// exact reference rollback and presentation independence.
template <std::size_t MaxFrames = 4096>
class ym2151_enhanced_recomposition_storage {
public:
    using engine_type = source_family_recomposition_storage<
        ym2151_recomposition_source_count,
        ym2151_recomposition_family_count,
        MaxFrames>;
    using source_array = typename engine_type::source_array;

    void reset() noexcept { engine_.reset(); }

    bool build(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count,
        const source_array& sources) noexcept
    {
        return engine_.build(reference_left, reference_right, frame_count, sources);
    }

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
    ym2151_enhanced_recomposition_error last_error() const noexcept {
        return engine_.last_error();
    }

    ym2151_recomposition_family_status family_status(
        ym2151_recomposition_family family) const noexcept
    {
        return engine_.family_status(static_cast<std::size_t>(family));
    }

    bool had_family_failure() const noexcept { return engine_.had_family_failure(); }

private:
    inline static constexpr typename engine_type::family_map source_families_{
        0, 0, 0, 0, 0, 0, 0, 0,
    };

    engine_type engine_{};
};

} // namespace gameaudio::vgm
