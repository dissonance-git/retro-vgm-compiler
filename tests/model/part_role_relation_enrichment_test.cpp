#include "model/part_role_relation_enrichment.h"
#include "model/part_role_window_inference.h"

#include <cassert>
#include <cmath>

using namespace vgmtooling::model;

namespace {

time_span window() {
    return time_span{
        time_coordinate{time_domain::source, 0, 1000, 0},
        time_coordinate{time_domain::source, 1000, 1000, 0},
    };
}

const inferred_part_role_candidate* find_candidate(
    const part_role_window_result& result,
    node_id part_id,
    musical_part_role role) {
    for (const auto& candidate : result.candidates) {
        if (candidate.part_id == part_id && candidate.role == role)
            return &candidate;
    }
    return nullptr;
}

} // namespace

int main() {
    part_role_window_descriptor bass;
    bass.part_id = 10;
    bass.active = window();
    bass.onset_count = 6;
    bass.register_coordinate = 2.0;
    bass.register_basis = "shared-pitch";

    bass_harmony_interaction_hypothesis bass_relation;
    bass_relation.kind = bass_harmony_interaction_kind::moving_bass_under_retained_upper_material;
    bass_relation.bass_part_id = 10;
    bass_relation.bass_identity_grounded = true;
    bass_relation.confidence = 0.91;
    enrich_role_descriptor_with_bass_harmony(bass, bass_relation);
    assert(bass.harmonic_bass_ownership.has_value());
    assert(std::fabs(bounded_role_signal_strength(*bass.harmonic_bass_ownership) - 0.91) < 1e-12);

    part_role_window_descriptor answer;
    answer.part_id = 20;
    answer.active = window();
    answer.onset_count = 5;
    answer.register_coordinate = 5.0;
    answer.register_basis = "shared-pitch";

    counterpoint_motion_profile counterpoint;
    counterpoint.first_part_id = 15;
    counterpoint.second_part_id = 20;
    counterpoint.contrary_motion_count = 4;
    counterpoint.confidence = 0.93;
    enrich_role_descriptor_with_counterpoint(answer, counterpoint);
    assert(answer.counterpoint_independence.has_value());
    assert(std::fabs(answer.counterpoint_independence->confidence - 0.72) < 1e-12);

    imitative_part_relation_hypothesis imitation;
    imitation.first_part_id = 15;
    imitation.second_part_id = 20;
    imitation.kind = imitative_part_relation_kind::imitation;
    imitation.confidence = 0.88;
    enrich_role_descriptor_with_imitation(answer, imitation);
    assert(answer.imitation_or_response.has_value());
    assert(std::fabs(answer.imitation_or_response->confidence - 0.88) < 1e-12);

    // Structural prominence and salience are separate evidence domains. The
    // enrichment API never derives prominence merely from local recurrence.
    enrich_role_descriptor_with_structural_motif_prominence(answer, 0.90, 0.95);
    enrich_role_descriptor_with_auditory_salience(answer, 0.82, 0.90);

    part_role_window_descriptor support;
    support.part_id = 30;
    support.active = window();
    support.onset_count = 3;
    support.register_coordinate = 4.0;
    support.register_basis = "shared-pitch";
    enrich_role_descriptor_with_sustained_texture(support, 0.90, 0.90);
    enrich_role_descriptor_with_auditory_salience(support, 0.15, 0.90);

    const auto roles = infer_part_roles_for_window(
        {bass, answer, support},
        "enriched-role-test");

    const auto* bass_candidate = find_candidate(
        roles,
        10,
        musical_part_role::bass_foundation);
    assert(bass_candidate != nullptr);
    assert(bass_candidate->hypothesis.confidence > 0.75);

    const auto* counterline = find_candidate(
        roles,
        20,
        musical_part_role::counterline);
    assert(counterline != nullptr);

    const auto* foreground = find_candidate(
        roles,
        20,
        musical_part_role::melodic_foreground);
    assert(foreground != nullptr);
    assert(foreground->hypothesis.cross_domain_grounded);
    assert(foreground->hypothesis.confidence > 0.75);

    const auto* sustained = find_candidate(
        roles,
        30,
        musical_part_role::sustained_support);
    assert(sustained != nullptr);
    assert(sustained->hypothesis.cross_domain_grounded);

    // A relation about some other part must not leak into this descriptor.
    part_role_window_descriptor unrelated;
    unrelated.part_id = 99;
    unrelated.active = window();
    enrich_role_descriptor_with_bass_harmony(unrelated, bass_relation);
    enrich_role_descriptor_with_counterpoint(unrelated, counterpoint);
    enrich_role_descriptor_with_imitation(unrelated, imitation);
    assert(!unrelated.harmonic_bass_ownership.has_value());
    assert(!unrelated.counterpoint_independence.has_value());
    assert(!unrelated.imitation_or_response.has_value());

    return 0;
}
