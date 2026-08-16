#include "model/imitative_part_relation.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

std::vector<part_gesture_observation> motif(
    node_id part_id,
    std::int64_t start_tick,
    double transpose_octaves,
    const char* basis,
    bool include_pitch = true) {
    const std::vector<double> shape = {
        0.0,
        2.0 / 12.0,
        4.0 / 12.0,
        5.0 / 12.0,
    };
    std::vector<part_gesture_observation> result;
    for (std::size_t index = 0; index < shape.size(); ++index) {
        part_gesture_observation observation;
        observation.source_node = static_cast<node_id>(100 + part_id * 10 + index);
        observation.part_id = part_id;
        observation.onset = {
            time_domain::source,
            start_tick + static_cast<std::int64_t>(index * 100),
            0,
            0,
        };
        if (include_pitch)
            observation.log2_pitch_coordinate = shape[index] + transpose_octaves;
        observation.pitch_basis = basis;
        observation.interval_semantics = "log2_frequency_ratio_octaves";
        result.push_back(std::move(observation));
    }
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    const auto call = motif(1, 0, 0.0, "part-a-native");
    const auto answer = motif(2, 150, 7.0 / 12.0, "part-b-native");

    // Native absolute bases differ, but each motif exposes the same derived
    // interval semantics. The transposed answer therefore remains recognizable
    // as imitation, and its entry lag is normalized against the initiating IOI.
    const auto imitation = infer_imitative_part_relation(call, answer);
    CHECK(imitation.kind == imitative_part_relation_kind::imitation);
    CHECK(imitation.first_part_id == 1);
    CHECK(imitation.second_part_id == 2);
    CHECK(imitation.onset_lag_ticks == 150);
    CHECK(close_enough(imitation.normalized_onset_lag, 1.5));
    CHECK(imitation.motif_similarity.pitch_comparable);
    CHECK(imitation.confidence > 0.99);

    // If pitch evidence disappears but the timing echo survives, the relation
    // remains observable but cannot inherit strong melodic-identity confidence.
    const auto rhythm_call = motif(3, 0, 0.0, "", false);
    const auto rhythm_answer = motif(4, 150, 0.0, "", false);
    const auto echo = infer_imitative_part_relation(rhythm_call, rhythm_answer);
    CHECK(echo.kind == imitative_part_relation_kind::rhythmic_echo);
    CHECK(!echo.motif_similarity.pitch_comparable);
    CHECK(echo.confidence <= rhythm_only_motif_identity_ceiling);

    return 0;
}
