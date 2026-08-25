#pragma once

#include "harmonic_verticality.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <stdexcept>
#include <vector>

namespace vgmtooling::model {

inline bool harmonic_timeline_same_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
        first.tick_rate == second.tick_rate &&
        first.loop_iteration == second.loop_iteration;
}

// Construct the changing harmonic surface from already-earned absolute musical
// pitch spans. Both newly attacked and already-sounding pitches participate in a
// verticality. Endpoints are end-exclusive, matching the underlying pitch-span
// contract: a state that ends at tick T is not sounding at T, while a successor
// beginning at T is. No interpolation or pitch estimation happens here.
//
// This layer intentionally does not infer chords, roots, bass function, key, or
// cadence. It only says which persistent musical parts have compatible absolute
// pitch evidence sounding together whenever that sounding set can change.
inline std::vector<harmonic_verticality> make_harmonic_verticality_timeline(
    const std::vector<absolute_musical_pitch_observation>& observations) {
    if (observations.empty())
        return {};

    const auto role = observations.front().role;
    const auto basis = observations.front().active.start;
    std::vector<std::int64_t> boundaries;
    boundaries.reserve(observations.size() * 2);

    for (const auto& observation : observations) {
        validate_absolute_musical_pitch_observation(observation);
        if (observation.role != role)
            throw std::invalid_argument(
                "harmonic timeline cannot mix programmed, performed, and heard pitch roles");
        if (!harmonic_timeline_same_time_basis(observation.active.start, basis))
            throw std::invalid_argument(
                "harmonic timeline pitch observations use incompatible time bases");
        if (!observation.active.end.has_value())
            throw std::invalid_argument(
                "harmonic timeline requires bounded pitch observations");
        if (!harmonic_timeline_same_time_basis(*observation.active.end, basis) ||
            observation.active.end->tick <= observation.active.start.tick) {
            throw std::invalid_argument(
                "harmonic timeline requires positive bounded spans in one time basis");
        }
        boundaries.push_back(observation.active.start.tick);
        boundaries.push_back(observation.active.end->tick);
    }

    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

    std::vector<harmonic_verticality> result;
    result.reserve(boundaries.size());
    for (const auto tick : boundaries) {
        time_coordinate time = basis;
        time.tick = tick;

        std::size_t active_count = 0;
        std::set<node_id> active_parts;
        for (const auto& observation : observations) {
            if (!time_coordinate_inside_span(time, observation.active))
                continue;
            ++active_count;
            if (!active_parts.insert(observation.part_id).second) {
                throw std::invalid_argument(
                    "harmonic timeline found simultaneous pitches for one persistent part");
            }
        }
        if (active_count < 2)
            continue;

        result.push_back(make_harmonic_verticality(time, observations));
    }
    return result;
}

} // namespace vgmtooling::model
