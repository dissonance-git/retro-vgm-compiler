#include "model/part_role_harmonic_bass_evidence_adapter.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

using namespace vgmtooling::model;

namespace {
constexpr node_id bass_part = 10;

part_role_window_descriptor descriptor_for(node_id part_id) {
    part_role_window_descriptor descriptor;
    descriptor.part_id = part_id;
    descriptor.active = {
        time_coordinate{time_domain::source, 0, 1000, 0},
        time_coordinate{time_domain::source, 4000, 1000, 0},
    };
    descriptor.onset_count = 4;
    return descriptor;
}

bass_harmony_interaction_hypothesis interaction(
    std::int64_t first_tick,
    std::int64_t second_tick,
    bass_harmony_interaction_kind kind,
    bool harmonic_change,
    double confidence = 0.90) {
    bass_harmony_interaction_hypothesis out;
    out.first_time = {time_domain::source, first_tick, 1000, 0};
    out.second_time = {time_domain::source, second_tick, 1000, 0};
    out.kind = kind;
    out.bass_part_id = bass_part;
    out.bass_identity_grounded = true;
    out.harmonic_identity_changed = harmonic_change;
    out.confidence = confidence;
    return out;
}
}

int main() {
    const std::vector<bass_harmony_interaction_hypothesis> static_harmony{
        interaction(0, 1000, bass_harmony_interaction_kind::harmonic_identity_retained, false),
        interaction(1000, 2000, bass_harmony_interaction_kind::harmonic_identity_retained, false),
        interaction(2000, 3000, bass_harmony_interaction_kind::harmonic_identity_retained, false),
    };
    const auto summary = infer_harmonic_bass_ownership(
        descriptor_for(bass_part), static_harmony);

    // This is the research-motivated desired behavior: unchanged-harmony
    // continuity alone must not mature into harmonic bass ownership.
    assert(summary.confidence < role_signal_use_threshold);
    return 0;
}
