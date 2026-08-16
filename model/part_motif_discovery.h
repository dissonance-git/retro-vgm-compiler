#pragma once

#include "part_motif_profile.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct part_motif_window {
    std::size_t start_index = 0;
    std::size_t event_count = 0;
    part_motif_profile profile;
};

struct repeated_part_motif_hypothesis {
    part_motif_window first;
    part_motif_window second;
    part_motif_similarity similarity;
};

struct part_motif_discovery_policy {
    std::size_t min_events = 3;
    std::size_t max_events = 8;
    double min_identity_confidence = 0.85;
    bool require_pitch_comparison = true;
};

inline void validate_part_motif_discovery_policy(const part_motif_discovery_policy& policy) {
    if (policy.min_events < 3)
        throw std::invalid_argument("motif discovery requires windows of at least three events");
    if (policy.max_events < policy.min_events)
        throw std::invalid_argument("motif discovery maximum window must not be smaller than its minimum");
    if (policy.min_identity_confidence < 0.0 || policy.min_identity_confidence > 1.0)
        throw std::invalid_argument("motif discovery confidence threshold must be in [0, 1]");
}

inline std::vector<part_motif_window> enumerate_part_motif_windows(
    const std::vector<part_gesture_observation>& observations,
    const part_motif_discovery_policy& policy = {}) {
    validate_part_motif_discovery_policy(policy);
    std::vector<part_motif_window> windows;
    if (observations.size() < policy.min_events)
        return windows;

    const std::size_t maximum = std::min(policy.max_events, observations.size());
    for (std::size_t event_count = policy.min_events; event_count <= maximum; ++event_count) {
        for (std::size_t start = 0; start + event_count <= observations.size(); ++start) {
            std::vector<part_gesture_observation> slice(
                observations.begin() + static_cast<std::ptrdiff_t>(start),
                observations.begin() + static_cast<std::ptrdiff_t>(start + event_count));
            try {
                windows.push_back({
                    start,
                    event_count,
                    make_part_motif_profile(slice),
                });
            } catch (const std::invalid_argument&) {
                // A window that crosses incompatible time/loop/part boundaries
                // is not repaired into a false musical pattern.
            }
        }
    }
    return windows;
}

inline bool motif_windows_overlap(
    const part_motif_window& first,
    const part_motif_window& second) noexcept {
    const std::size_t first_end = first.start_index + first.event_count;
    const std::size_t second_end = second.start_index + second.event_count;
    return first.start_index < second_end && second.start_index < first_end;
}

inline std::vector<repeated_part_motif_hypothesis> discover_repeated_part_motifs(
    const std::vector<part_gesture_observation>& observations,
    const part_motif_discovery_policy& policy = {}) {
    validate_part_motif_discovery_policy(policy);
    const auto windows = enumerate_part_motif_windows(observations, policy);
    std::vector<repeated_part_motif_hypothesis> results;

    for (std::size_t first_index = 0; first_index < windows.size(); ++first_index) {
        for (std::size_t second_index = first_index + 1; second_index < windows.size(); ++second_index) {
            const auto& first = windows[first_index];
            const auto& second = windows[second_index];
            if (first.event_count != second.event_count || motif_windows_overlap(first, second))
                continue;

            const auto similarity = compare_part_motif_profiles(first.profile, second.profile);
            if (policy.require_pitch_comparison && !similarity.pitch_comparable)
                continue;
            if (similarity.identity_confidence < policy.min_identity_confidence)
                continue;

            results.push_back({first, second, similarity});
        }
    }

    std::sort(results.begin(), results.end(), [](const auto& first, const auto& second) {
        if (first.similarity.identity_confidence != second.similarity.identity_confidence)
            return first.similarity.identity_confidence > second.similarity.identity_confidence;
        if (first.first.event_count != second.first.event_count)
            return first.first.event_count > second.first.event_count;
        if (first.first.start_index != second.first.start_index)
            return first.first.start_index < second.first.start_index;
        return first.second.start_index < second.second.start_index;
    });
    return results;
}

} // namespace vgmtooling::model
