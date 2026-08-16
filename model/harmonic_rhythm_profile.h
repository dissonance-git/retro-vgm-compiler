#pragma once

#include "harmonic_transition_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct harmonic_rhythm_change {
    time_coordinate time{};
    std::int64_t root_pitch_class = 0;
    tertian_triad_quality quality = tertian_triad_quality::major;
    double confidence = 0.0;
};

struct harmonic_rhythm_profile {
    std::vector<harmonic_rhythm_change> changes;
    std::vector<std::int64_t> change_gaps_ticks;
    std::vector<double> normalized_change_gaps;
    double normalization_gap_ticks = 0.0;
    double confidence = 0.0;
};

inline bool same_harmonic_identity(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second) noexcept {
    return first.root_pitch_class == second.root_pitch_class &&
        first.quality == second.quality;
}

inline double median_positive_gap(std::vector<std::int64_t> gaps) {
    gaps.erase(
        std::remove_if(gaps.begin(), gaps.end(), [](std::int64_t gap) {
            return gap <= 0;
        }),
        gaps.end());
    if (gaps.empty())
        throw std::invalid_argument("harmonic rhythm requires at least one positive change gap");
    std::sort(gaps.begin(), gaps.end());
    const std::size_t middle = gaps.size() / 2;
    if ((gaps.size() & 1u) != 0u)
        return static_cast<double>(gaps[middle]);
    return 0.5 * static_cast<double>(gaps[middle - 1] + gaps[middle]);
}

inline harmonic_rhythm_profile make_harmonic_rhythm_profile(
    std::vector<tertian_triad_hypothesis> observations) {
    if (observations.size() < 2)
        throw std::invalid_argument("harmonic rhythm requires at least two chord observations");

    std::sort(observations.begin(), observations.end(), [](const auto& first, const auto& second) {
        return first.projection.source_verticality.observation_time.tick <
            second.projection.source_verticality.observation_time.tick;
    });

    const time_coordinate basis = observations.front().projection.source_verticality.observation_time;
    for (const auto& observation : observations) {
        const time_coordinate time = observation.projection.source_verticality.observation_time;
        if (time.domain != basis.domain ||
            time.tick_rate != basis.tick_rate ||
            time.loop_iteration != basis.loop_iteration) {
            throw std::invalid_argument("harmonic rhythm requires one compatible local time basis");
        }
    }

    harmonic_rhythm_profile result;
    result.confidence = 1.0;

    const tertian_triad_hypothesis* previous_identity = nullptr;
    std::int64_t previous_tick = std::numeric_limits<std::int64_t>::min();
    for (const auto& observation : observations) {
        const time_coordinate time = observation.projection.source_verticality.observation_time;
        if (time.tick <= previous_tick)
            throw std::invalid_argument("harmonic rhythm chord observations must have unique increasing times");
        previous_tick = time.tick;

        if (previous_identity != nullptr && same_harmonic_identity(*previous_identity, observation))
            continue;

        result.changes.push_back({
            time,
            observation.root_pitch_class,
            observation.quality,
            observation.confidence,
        });
        result.confidence = std::min(result.confidence, observation.confidence);
        previous_identity = &observation;
    }

    if (result.changes.size() < 2)
        throw std::invalid_argument("harmonic rhythm requires at least two distinct harmonic identities");

    result.change_gaps_ticks.reserve(result.changes.size() - 1);
    for (std::size_t index = 1; index < result.changes.size(); ++index) {
        const std::int64_t gap = result.changes[index].time.tick - result.changes[index - 1].time.tick;
        if (gap <= 0)
            throw std::invalid_argument("harmonic rhythm contains a non-positive change gap");
        result.change_gaps_ticks.push_back(gap);
    }

    result.normalization_gap_ticks = median_positive_gap(result.change_gaps_ticks);
    result.normalized_change_gaps.reserve(result.change_gaps_ticks.size());
    for (std::int64_t gap : result.change_gaps_ticks) {
        result.normalized_change_gaps.push_back(
            static_cast<double>(gap) / result.normalization_gap_ticks);
    }
    return result;
}

inline double harmonic_rhythm_similarity(
    const harmonic_rhythm_profile& first,
    const harmonic_rhythm_profile& second) {
    if (first.normalized_change_gaps.empty() || second.normalized_change_gaps.empty())
        return 0.0;
    if (first.normalized_change_gaps.size() != second.normalized_change_gaps.size())
        return 0.0;

    double error = 0.0;
    for (std::size_t index = 0; index < first.normalized_change_gaps.size(); ++index) {
        const double lhs = first.normalized_change_gaps[index];
        const double rhs = second.normalized_change_gaps[index];
        if (lhs <= 0.0 || rhs <= 0.0)
            return 0.0;
        error += std::fabs(std::log2(lhs / rhs));
    }
    error /= static_cast<double>(first.normalized_change_gaps.size());

    // An octave of timing-ratio error averages to zero similarity. Exact
    // proportional timing survives global tempo scaling and scores 1.0.
    return std::max(0.0, 1.0 - error);
}

} // namespace vgmtooling::model
