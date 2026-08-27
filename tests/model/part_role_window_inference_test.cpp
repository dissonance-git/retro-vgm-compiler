#include "model/part_role_window_inference.h"

#include <cassert>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

time_span window() {
    return time_span{
        time_coordinate{time_domain::source, 0, 1000, 0},
        time_coordinate{time_domain::source, 1000, 1000, 0},
    };
}

bounded_role_signal signal(double value, double confidence = 1.0) {
    return {value, confidence};
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
    part_role_window_descriptor lead;
    lead.part_id = 10;
    lead.active = window();
    lead.onset_count = 8;
    lead.register_coordinate = 6.0;
    lead.register_basis = "shared-log2-pitch";
    lead.auditory_salience = signal(0.92, 0.95);
    lead.structural_motif_prominence = signal(0.95, 0.95);
    lead.phrase_boundary_participation = signal(0.85, 0.90);

    part_role_window_descriptor bass;
    bass.part_id = 20;
    bass.active = window();
    bass.onset_count = 7;
    bass.register_coordinate = 2.0;
    bass.register_basis = "shared-log2-pitch";
    bass.auditory_salience = signal(0.20, 0.90);
    bass.harmonic_bass_ownership = signal(0.98, 0.95);
    bass.rhythmic_repetition = signal(0.85, 0.90);

    part_role_window_descriptor answer;
    answer.part_id = 30;
    answer.active = window();
    answer.onset_count = 5;
    answer.register_coordinate = 4.5;
    answer.register_basis = "shared-log2-pitch";
    answer.auditory_salience = signal(0.70, 0.90);
    answer.counterpoint_independence = signal(0.90, 0.90);
    answer.imitation_or_response = signal(0.90, 0.90);

    const auto roles = infer_part_roles_for_window(
        {lead, bass, answer},
        "automatic-role-test");

    const auto* foreground = find_candidate(
        roles,
        lead.part_id,
        musical_part_role::melodic_foreground);
    assert(foreground != nullptr);
    assert(foreground->hypothesis.relationally_grounded);
    assert(foreground->hypothesis.cross_domain_grounded);
    assert(foreground->hypothesis.confidence > 0.75);

    const auto* bass_role = find_candidate(
        roles,
        bass.part_id,
        musical_part_role::bass_foundation);
    assert(bass_role != nullptr);
    assert(bass_role->hypothesis.relationally_grounded);
    assert(bass_role->hypothesis.cross_domain_grounded);
    assert(bass_role->hypothesis.confidence > 0.75);

    // The bass can simultaneously be an ostinato. Roles are analytical
    // functions, not a one-hot identity partition.
    const auto* bass_ostinato = find_candidate(
        roles,
        bass.part_id,
        musical_part_role::ostinato);
    assert(bass_ostinato != nullptr);
    assert(bass_ostinato->hypothesis.confidence < bass_role->hypothesis.confidence);

    const auto* accompaniment = find_candidate(
        roles,
        bass.part_id,
        musical_part_role::accompaniment);
    assert(accompaniment != nullptr);
    assert(accompaniment->hypothesis.confidence >= role_signal_use_threshold);

    const auto* counterline = find_candidate(
        roles,
        answer.part_id,
        musical_part_role::counterline);
    assert(counterline != nullptr);
    assert(counterline->hypothesis.relationally_grounded);
    assert(counterline->hypothesis.cross_domain_grounded);
    // response=.81 and capped counterpoint=.72 are the two strongest supports;
    // weaker salience remains provenance without diluting the proposal.
    assert(std::fabs(counterline->hypothesis.confidence - 0.765) < 1e-9);
    assert(find_candidate(
        roles,
        answer.part_id,
        musical_part_role::melodic_foreground) == nullptr);

    // Being uniquely high, busy, and salient does not create melodic foreground
    // without structural motif evidence.
    part_role_window_descriptor superficial;
    superficial.part_id = 40;
    superficial.active = window();
    superficial.onset_count = 10;
    superficial.register_coordinate = 9.0;
    superficial.register_basis = "shared-log2-pitch";
    superficial.auditory_salience = signal(1.0, 1.0);

    part_role_window_descriptor quiet;
    quiet.part_id = 41;
    quiet.active = window();
    quiet.onset_count = 2;
    quiet.register_coordinate = 1.0;
    quiet.register_basis = "shared-log2-pitch";

    const auto superficial_roles = infer_part_roles_for_window(
        {superficial, quiet},
        "superficial-role-test");
    assert(find_candidate(
        superficial_roles,
        superficial.part_id,
        musical_part_role::melodic_foreground) == nullptr);

    // Rhythmic repetition never manufactures percussion identity.
    superficial.rhythmic_repetition = signal(0.99, 0.99);
    const auto rhythmic_roles = infer_part_roles_for_window(
        {superficial, quiet},
        "rhythmic-role-test");
    assert(find_candidate(
        rhythmic_roles,
        superficial.part_id,
        musical_part_role::percussion_pulse) == nullptr);

    superficial.percussion_identity = signal(0.95, 0.95);
    const auto percussion_roles = infer_part_roles_for_window(
        {superficial, quiet},
        "percussion-role-test");
    assert(find_candidate(
        percussion_roles,
        superficial.part_id,
        musical_part_role::percussion_pulse) != nullptr);

    // Incompatible register bases remove register-ranking evidence instead of
    // forcing a common pitch system. Harmonic ownership can still support bass,
    // but without an independent domain it remains below the strong threshold.
    part_role_window_descriptor cross_a = bass;
    cross_a.part_id = 50;
    cross_a.register_basis = "genesis-relative";
    cross_a.rhythmic_repetition.reset();
    cross_a.auditory_salience.reset();
    part_role_window_descriptor cross_b = lead;
    cross_b.part_id = 51;
    cross_b.register_basis = "spc-same-sample";
    cross_b.structural_motif_prominence.reset();
    cross_b.phrase_boundary_participation.reset();
    cross_b.auditory_salience.reset();

    const auto cross_basis_roles = infer_part_roles_for_window(
        {cross_a, cross_b},
        "cross-basis-role-test");
    const auto* cross_bass = find_candidate(
        cross_basis_roles,
        cross_a.part_id,
        musical_part_role::bass_foundation);
    assert(cross_bass != nullptr);
    assert(!cross_bass->hypothesis.cross_domain_grounded);
    assert(cross_bass->hypothesis.confidence <= part_role_single_domain_ceiling);

    return 0;
}
