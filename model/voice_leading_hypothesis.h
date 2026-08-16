#pragma once

#include "tertian_triad_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct voice_leading_motion {
    std::int64_t first_step = 0;
    std::int64_t second_step = 0;
    std::int64_t semitone_motion = 0;
    node_id first_part_id = 0;
    node_id second_part_id = 0;
    bool persistent_identity_preserved = false;
};

struct voice_leading_hypothesis {
    time_coordinate first_time{};
    time_coordinate second_time{};
    std::vector<voice_leading_motion> motions;
    std::int64_t total_absolute_motion_semitones = 0;
    std::size_t stationary_voices = 0;
    std::size_t upward_voices = 0;
    std::size_t downward_voices = 0;
    std::size_t identity_preserved_voices = 0;
    bool all_correspondence_identity_grounded = false;
    bool fallback_assignment_ambiguous = false;
    double confidence = 0.0;
};

constexpr double inferred_voice_correspondence_ceiling = 0.78;
constexpr double ambiguous_voice_correspondence_ceiling = 0.60;

inline void validate_voice_leading_projection(
    const tertian_triad_hypothesis& chord) {
    const auto& projection = chord.projection;
    if (projection.tuning.divisions_per_octave != 12)
        throw std::invalid_argument("voice-leading semitone analysis currently requires an explicit 12-TET projection");
    if (projection.nearest_steps.size() < 2)
        throw std::invalid_argument("voice leading requires at least two projected pitches per verticality");
    if (projection.nearest_steps.size() != projection.source_verticality.part_ids.size())
        throw std::invalid_argument("voice leading requires one persistent-part slot for every projected pitch");

    std::set<node_id> nonzero_parts;
    for (node_id part_id : projection.source_verticality.part_ids) {
        if (part_id == 0)
            continue;
        if (!nonzero_parts.insert(part_id).second)
            throw std::invalid_argument("voice leading requires unique nonzero part identities within one verticality");
    }
}

inline voice_leading_hypothesis infer_voice_leading(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second) {
    validate_voice_leading_projection(first);
    validate_voice_leading_projection(second);

    const auto& first_projection = first.projection;
    const auto& second_projection = second.projection;
    if (first_projection.nearest_steps.size() != second_projection.nearest_steps.size())
        throw std::invalid_argument("voice leading currently requires equal voice cardinality");

    const time_coordinate first_time = first_projection.source_verticality.observation_time;
    const time_coordinate second_time = second_projection.source_verticality.observation_time;
    if (first_time.domain != second_time.domain ||
        first_time.tick_rate != second_time.tick_rate ||
        first_time.loop_iteration != second_time.loop_iteration ||
        second_time.tick <= first_time.tick) {
        throw std::invalid_argument("voice leading requires ordered verticalities in one compatible time basis");
    }

    voice_leading_hypothesis result;
    result.first_time = first_time;
    result.second_time = second_time;
    result.confidence = std::min(first.confidence, second.confidence);

    std::map<node_id, std::size_t> second_by_part;
    for (std::size_t index = 0; index < second_projection.source_verticality.part_ids.size(); ++index) {
        const node_id part_id = second_projection.source_verticality.part_ids[index];
        if (part_id != 0)
            second_by_part.emplace(part_id, index);
    }

    std::vector<bool> first_used(first_projection.nearest_steps.size(), false);
    std::vector<bool> second_used(second_projection.nearest_steps.size(), false);

    // Persistent musical-part identity outranks geometric pitch proximity.
    for (std::size_t first_index = 0; first_index < first_projection.nearest_steps.size(); ++first_index) {
        const node_id part_id = first_projection.source_verticality.part_ids[first_index];
        if (part_id == 0)
            continue;
        const auto match = second_by_part.find(part_id);
        if (match == second_by_part.end())
            continue;

        const std::size_t second_index = match->second;
        const std::int64_t motion =
            second_projection.nearest_steps[second_index] - first_projection.nearest_steps[first_index];
        result.motions.push_back({
            first_projection.nearest_steps[first_index],
            second_projection.nearest_steps[second_index],
            motion,
            part_id,
            part_id,
            true,
        });
        first_used[first_index] = true;
        second_used[second_index] = true;
        ++result.identity_preserved_voices;
    }

    std::vector<std::size_t> remaining_first;
    std::vector<std::size_t> remaining_second;
    for (std::size_t index = 0; index < first_used.size(); ++index) {
        if (!first_used[index])
            remaining_first.push_back(index);
        if (!second_used[index])
            remaining_second.push_back(index);
    }
    if (remaining_first.size() != remaining_second.size())
        throw std::logic_error("voice-leading unmatched voice cardinality diverged");

    if (!remaining_first.empty()) {
        std::sort(remaining_second.begin(), remaining_second.end());
        std::vector<std::size_t> best_assignment = remaining_second;
        std::int64_t best_cost = std::numeric_limits<std::int64_t>::max();
        std::size_t best_count = 0;

        do {
            std::int64_t cost = 0;
            for (std::size_t index = 0; index < remaining_first.size(); ++index) {
                cost += std::llabs(
                    second_projection.nearest_steps[remaining_second[index]] -
                    first_projection.nearest_steps[remaining_first[index]]);
            }
            if (cost < best_cost) {
                best_cost = cost;
                best_assignment = remaining_second;
                best_count = 1;
            } else if (cost == best_cost) {
                ++best_count;
            }
        } while (std::next_permutation(remaining_second.begin(), remaining_second.end()));

        result.fallback_assignment_ambiguous = best_count > 1;
        result.confidence = std::min(
            result.confidence,
            result.fallback_assignment_ambiguous
                ? ambiguous_voice_correspondence_ceiling
                : inferred_voice_correspondence_ceiling);

        for (std::size_t index = 0; index < remaining_first.size(); ++index) {
            const std::size_t first_index = remaining_first[index];
            const std::size_t second_index = best_assignment[index];
            const std::int64_t motion =
                second_projection.nearest_steps[second_index] - first_projection.nearest_steps[first_index];
            result.motions.push_back({
                first_projection.nearest_steps[first_index],
                second_projection.nearest_steps[second_index],
                motion,
                first_projection.source_verticality.part_ids[first_index],
                second_projection.source_verticality.part_ids[second_index],
                false,
            });
        }
    }

    result.all_correspondence_identity_grounded =
        result.identity_preserved_voices == result.motions.size();

    for (const auto& motion : result.motions) {
        result.total_absolute_motion_semitones += std::llabs(motion.semitone_motion);
        if (motion.semitone_motion == 0)
            ++result.stationary_voices;
        else if (motion.semitone_motion > 0)
            ++result.upward_voices;
        else
            ++result.downward_voices;
    }
    return result;
}

} // namespace vgmtooling::model
