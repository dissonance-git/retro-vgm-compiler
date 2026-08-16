#include "model/orchestration_transition_hypothesis.h"
#include "model/part_role_evidence_bridge.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace vgmtooling::model;

namespace {

time_span span(std::int64_t begin, std::int64_t end) {
    return time_span{
        time_coordinate{time_domain::source, begin, 44100, 0},
        time_coordinate{time_domain::source, end, 44100, 0},
    };
}

part_role_evidence relational(
    part_role_evidence_kind kind,
    double confidence,
    std::string detail) {
    return {
        kind,
        part_role_evidence_origin::musical_analysis,
        part_role_evidence_polarity::supports,
        evidence_status::hypothesis,
        confidence,
        "orchestration-test",
        std::move(detail),
        {},
    };
}

musical_part_role_hypothesis role(
    node_id part,
    musical_part_role kind,
    std::int64_t begin,
    std::int64_t end,
    double confidence) {
    return make_musical_part_role_hypothesis(
        part,
        kind,
        span(begin, end),
        confidence,
        {
            relational(part_role_evidence_kind::melodic_motif_prominence, confidence, "structural material"),
            relational(part_role_evidence_kind::phrase_initiation_or_completion, confidence, "phrase position"),
        });
}

orchestration_realization realization(
    std::string basis,
    std::string identity,
    double confidence = 0.90) {
    return {
        std::move(basis),
        std::move(identity),
        evidence_status::derived,
        confidence,
        "orchestration-test",
    };
}

} // namespace

int main() {
    const auto first_role = role(10, musical_part_role::melodic_foreground, 0, 100, 0.86);
    const auto second_role = role(10, musical_part_role::melodic_foreground, 100, 200, 0.84);

    const auto first = make_part_orchestration_state(
        first_role,
        realization("ym2612_program", "patch-a"),
        4.0,
        "log2_hz",
        0.50);
    const auto recolored = make_part_orchestration_state(
        second_role,
        realization("ym2612_program", "patch-b"),
        4.0,
        "log2_hz",
        0.50);

    const auto timbre = infer_orchestration_transition(first, recolored);
    assert(timbre.kind == orchestration_transition_kind::timbral_recoloring);
    assert(timbre.persistent_part_preserved);
    assert(timbre.role_preserved);
    assert(timbre.realization_comparable);
    assert(timbre.timbre_changed);

    const auto raised = make_part_orchestration_state(
        second_role,
        realization("ym2612_program", "patch-a"),
        5.0,
        "log2_hz",
        0.50);
    const auto register_change = infer_orchestration_transition(first, raised);
    assert(register_change.kind == orchestration_transition_kind::registral_revoicing);
    assert(register_change.register_comparable);
    assert(std::fabs(register_change.register_shift - 1.0) < 1e-12);

    const auto supporting_role = role(10, musical_part_role::accompaniment, 100, 200, 0.82);
    const auto changed_job = make_part_orchestration_state(
        supporting_role,
        realization("ym2612_program", "patch-b"),
        3.5,
        "log2_hz",
        0.35);
    const auto compound = infer_orchestration_transition(first, changed_job);
    assert(compound.kind == orchestration_transition_kind::compound_reorchestration);
    assert(compound.persistent_part_preserved);
    assert(!compound.role_preserved);

    // A role appearing on another part is not a transfer merely because the
    // role label matches. Musical-material continuity must be grounded first.
    const auto other_role = role(11, musical_part_role::melodic_foreground, 100, 200, 0.85);
    const auto other = make_part_orchestration_state(
        other_role,
        realization("brr_sample_version", "sample-17"),
        5.0,
        "log2_hz",
        0.55);
    const auto ungrounded_transfer = infer_orchestration_transition(first, other);
    assert(ungrounded_transfer.kind == orchestration_transition_kind::unresolved);
    assert(!ungrounded_transfer.musical_material_continuity_grounded);
    assert(ungrounded_transfer.confidence <= ungrounded_cross_part_orchestration_ceiling);

    const auto transfer = infer_orchestration_transition(first, other, 0.82);
    assert(transfer.kind == orchestration_transition_kind::role_transfer);
    assert(transfer.musical_material_continuity_grounded);
    assert(std::fabs(transfer.confidence - 0.82) < 1e-12);

    // Opaque source-native realization identities are comparable only inside
    // the same basis. Equal strings across source families do not imply timbre
    // equality or difference.
    const auto different_basis = make_part_orchestration_state(
        second_role,
        realization("brr_sample_version", "patch-a"),
        4.0,
        "log2_hz",
        0.50);
    const auto basis_guard = infer_orchestration_transition(first, different_basis);
    assert(!basis_guard.realization_comparable);
    assert(!basis_guard.timbre_changed);
    assert(basis_guard.kind == orchestration_transition_kind::stable_assignment);

    // Existing musical relations can feed role evidence without turning a
    // physical coordinate into role identity.
    bass_harmony_interaction_hypothesis bass;
    bass.kind = bass_harmony_interaction_kind::moving_bass_under_retained_upper_material;
    bass.bass_part_id = 22;
    bass.bass_identity_grounded = true;
    bass.confidence = 0.88;
    const auto bass_evidence = bass_foundation_evidence_from_harmony(
        bass,
        22,
        "bass-harmony-analysis");
    assert(bass_evidence.kind == part_role_evidence_kind::harmonic_bass_ownership);
    assert(std::fabs(bass_evidence.confidence - 0.88) < 1e-12);

    counterpoint_motion_profile counterpoint;
    counterpoint.first_part_id = 30;
    counterpoint.second_part_id = 31;
    counterpoint.contrary_motion_count = 3;
    counterpoint.confidence = 0.94;
    const auto counterline = counterline_evidence_from_counterpoint(
        counterpoint,
        31,
        "counterpoint-analysis");
    assert(counterline.kind == part_role_evidence_kind::counterpoint_independence);
    assert(std::fabs(counterline.confidence - 0.72) < 1e-12);

    imitative_part_relation_hypothesis imitation;
    imitation.first_part_id = 30;
    imitation.second_part_id = 31;
    imitation.kind = imitative_part_relation_kind::imitation;
    imitation.confidence = 0.89;
    const auto response = response_evidence_from_imitation(
        imitation,
        31,
        "imitation-analysis");
    assert(response.kind == part_role_evidence_kind::imitation_or_response);
    assert(std::fabs(response.confidence - 0.89) < 1e-12);

    return 0;
}
