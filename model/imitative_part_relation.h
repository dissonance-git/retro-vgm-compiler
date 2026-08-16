#pragma once

#include "part_motif_profile.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace vgmtooling::model {

enum class imitative_part_relation_kind : std::uint8_t {
    imitation = 0,
    rhythmic_echo,
    weak_relation,
};

struct imitative_part_relation_hypothesis {
    node_id first_part_id = 0;
    node_id second_part_id = 0;
    imitative_part_relation_kind kind = imitative_part_relation_kind::weak_relation;
    std::int64_t onset_lag_ticks = 0;
    double normalized_onset_lag = 0.0;
    part_motif_similarity motif_similarity{};
    double confidence = 0.0;
};

constexpr double imitative_relation_threshold = 0.75;
constexpr double rhythmic_echo_threshold = 0.80;

inline const char* to_string(imitative_part_relation_kind kind) noexcept {
    switch (kind) {
    case imitative_part_relation_kind::imitation:
        return "imitation";
    case imitative_part_relation_kind::rhythmic_echo:
        return "rhythmic_echo";
    case imitative_part_relation_kind::weak_relation:
        return "weak_relation";
    }
    return "unknown";
}

inline imitative_part_relation_hypothesis infer_imitative_part_relation(
    const std::vector<part_gesture_observation>& first,
    const std::vector<part_gesture_observation>& second) {
    if (first.size() < 3 || first.size() != second.size())
        throw std::invalid_argument("imitative relation requires equally sized motif observation windows");
    if (first.front().part_id == 0 || second.front().part_id == 0 ||
        first.front().part_id == second.front().part_id) {
        throw std::invalid_argument("imitative relation requires two distinct persistent parts");
    }
    if (first.front().onset.domain != second.front().onset.domain ||
        first.front().onset.tick_rate != second.front().onset.tick_rate ||
        first.front().onset.loop_iteration != second.front().onset.loop_iteration) {
        throw std::invalid_argument("imitative relation requires one compatible local time basis");
    }
    if (second.front().onset.tick < first.front().onset.tick)
        throw std::invalid_argument("imitative relation expects the responding part to begin no earlier than the initiating part");

    const auto first_profile = make_part_motif_profile(first);
    const auto second_profile = make_part_motif_profile(second);
    const auto similarity = compare_part_motif_profiles(first_profile, second_profile);

    std::vector<double> first_iois;
    first_iois.reserve(first.size() - 1);
    for (std::size_t index = 1; index < first.size(); ++index) {
        const auto delta = first[index].onset.tick - first[index - 1].onset.tick;
        if (delta <= 0)
            throw std::invalid_argument("imitative relation initiating motif must have increasing onsets");
        first_iois.push_back(static_cast<double>(delta));
    }
    const double reference_ioi = median_positive(first_iois);

    imitative_part_relation_hypothesis result;
    result.first_part_id = first.front().part_id;
    result.second_part_id = second.front().part_id;
    result.onset_lag_ticks = second.front().onset.tick - first.front().onset.tick;
    result.normalized_onset_lag =
        static_cast<double>(result.onset_lag_ticks) / reference_ioi;
    result.motif_similarity = similarity;
    result.confidence = similarity.identity_confidence;

    if (similarity.pitch_comparable &&
        similarity.identity_confidence >= imitative_relation_threshold) {
        result.kind = imitative_part_relation_kind::imitation;
    } else if (!similarity.pitch_comparable &&
               similarity.rhythm_similarity >= rhythmic_echo_threshold) {
        result.kind = imitative_part_relation_kind::rhythmic_echo;
        result.confidence = std::min(
            result.confidence,
            rhythm_only_motif_identity_ceiling);
    } else {
        result.kind = imitative_part_relation_kind::weak_relation;
    }
    return result;
}

} // namespace vgmtooling::model
