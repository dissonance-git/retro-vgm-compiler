#include "model/part_role_harmonic_bass_evidence_adapter.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

constexpr node_id bass_part = 10;
constexpr node_id upper_part_a = 20;
constexpr node_id upper_part_b = 30;

tertian_triad_hypothesis make_chord(
    std::int64_t tick,
    std::int64_t root_pitch_class,
    tertian_triad_quality quality,
    triad_inversion inversion,
    std::vector<std::int64_t> nearest_steps,
    std::vector<node_id> part_ids,
    double confidence = 0.90) {
    tertian_triad_hypothesis chord;
    chord.root_pitch_class = root_pitch_class;
    chord.quality = quality;
    chord.inversion = inversion;
    chord.confidence = confidence;
    chord.projection.confidence = confidence;
    chord.projection.tuning.divisions_per_octave = 12;
    chord.projection.tuning.confidence = 1.0;
    chord.projection.tuning.source = "harmonic-bass-role-test";
    chord.projection.source_verticality.observation_time = {
        time_domain::source,
        tick,
        1000,
        0,
    };
    chord.projection.source_verticality.part_ids = std::move(part_ids);
    chord.projection.nearest_steps = std::move(nearest_steps);
    return chord;
}

bass_harmony_interaction_hypothesis connect(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second) {
    const auto voices = infer_voice_leading(first, second);
    assert(voices.all_correspondence_identity_grounded);
    return infer_bass_harmony_interaction(first, second, voices);
}

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

bool contains_role(
    const part_role_window_result& result,
    node_id part_id,
    musical_part_role role) {
    for (const auto& candidate : result.candidates) {
        if (candidate.part_id == part_id && candidate.role == role)
            return true;
    }
    return false;
}

} // namespace

int main() {
    // One persistent part remains the sounding bass while the same C-major
    // harmony moves through root position, first inversion, and second
    // inversion, then changes to F major. This is deliberately not a
    // root-equals-bass test: on the middle two sonorities the bass pitch class
    // is E and G while the harmonic root remains C.
    const auto c_root = make_chord(
        0,
        0,
        tertian_triad_quality::major,
        triad_inversion::root_position,
        {36, 52, 55},
        {bass_part, upper_part_a, upper_part_b});
    const auto c_first = make_chord(
        1000,
        0,
        tertian_triad_quality::major,
        triad_inversion::first,
        {40, 55, 60},
        {bass_part, upper_part_b, upper_part_a});
    const auto c_second = make_chord(
        2000,
        0,
        tertian_triad_quality::major,
        triad_inversion::second,
        {43, 48, 52},
        {bass_part, upper_part_a, upper_part_b});
    const auto f_root = make_chord(
        3000,
        5,
        tertian_triad_quality::major,
        triad_inversion::root_position,
        {41, 48, 57},
        {bass_part, upper_part_a, upper_part_b});

    assert(positive_mod(c_root.projection.nearest_steps.front(), 12) == c_root.root_pitch_class);
    assert(positive_mod(c_first.projection.nearest_steps.front(), 12) != c_first.root_pitch_class);
    assert(positive_mod(c_second.projection.nearest_steps.front(), 12) != c_second.root_pitch_class);

    const auto first_transition = connect(c_root, c_first);
    const auto second_transition = connect(c_first, c_second);
    const auto third_transition = connect(c_second, f_root);

    assert(first_transition.bass_identity_grounded);
    assert(second_transition.bass_identity_grounded);
    assert(third_transition.bass_identity_grounded);
    assert(first_transition.bass_part_id == bass_part);
    assert(second_transition.bass_part_id == bass_part);
    assert(third_transition.bass_part_id == bass_part);
    assert(first_transition.kind == bass_harmony_interaction_kind::inversion_or_bass_revoicing);
    assert(second_transition.kind == bass_harmony_interaction_kind::inversion_or_bass_revoicing);
    assert(third_transition.kind == bass_harmony_interaction_kind::moving_bass_under_retained_upper_material);

    const std::vector<bass_harmony_interaction_hypothesis> interactions{
        first_transition,
        second_transition,
        third_transition,
    };

    auto bass_descriptor = descriptor_for(bass_part);
    const auto bass_summary = infer_harmonic_bass_ownership(
        bass_descriptor,
        interactions);
    assert(bass_summary.eligible_transitions == 3);
    assert(bass_summary.grounded_transitions == 3);
    assert(bass_summary.owned_transitions == 3);
    assert(std::fabs(bass_summary.ownership_fraction - 1.0) < 1.0e-12);
    assert(std::fabs(bass_summary.confidence - 0.90) < 1.0e-12);
    assert(attach_harmonic_bass_ownership(bass_descriptor, interactions));
    assert(bass_descriptor.harmonic_bass_ownership.has_value());
    assert(role_signal_strength_if_usable(
        bass_descriptor.harmonic_bass_ownership).has_value());

    const auto bass_roles = infer_part_roles_for_window(
        {bass_descriptor},
        "harmonic-bass-role-test");
    assert(contains_role(
        bass_roles,
        bass_part,
        musical_part_role::bass_foundation));

    // A simultaneous upper part sees the same harmonic transitions but does not
    // own the bass relation. Knowing that it is not the owner remains a bounded
    // zero-valued signal rather than being confused with missing evidence.
    auto upper_descriptor = descriptor_for(upper_part_a);
    const auto upper_summary = infer_harmonic_bass_ownership(
        upper_descriptor,
        interactions);
    assert(upper_summary.grounded_transitions == 3);
    assert(upper_summary.owned_transitions == 0);
    assert(std::fabs(upper_summary.ownership_fraction) < 1.0e-12);
    assert(attach_harmonic_bass_ownership(upper_descriptor, interactions));
    assert(upper_descriptor.harmonic_bass_ownership.has_value());
    assert(!role_signal_strength_if_usable(
        upper_descriptor.harmonic_bass_ownership).has_value());
    const auto upper_roles = infer_part_roles_for_window(
        {upper_descriptor},
        "harmonic-bass-role-test");
    assert(!contains_role(
        upper_roles,
        upper_part_a,
        musical_part_role::bass_foundation));

    // One transition is not enough history to promote register position into a
    // harmonic bass role. Support maturity remains deliberately below the role
    // kernel's use threshold.
    auto sparse_descriptor = descriptor_for(bass_part);
    const std::vector<bass_harmony_interaction_hypothesis> sparse{
        first_transition,
    };
    const auto sparse_summary = infer_harmonic_bass_ownership(
        sparse_descriptor,
        sparse);
    assert(sparse_summary.grounded_transitions == 1);
    assert(sparse_summary.confidence < role_signal_use_threshold);
    assert(attach_harmonic_bass_ownership(sparse_descriptor, sparse));
    assert(!role_signal_strength_if_usable(
        sparse_descriptor.harmonic_bass_ownership).has_value());
    const auto sparse_roles = infer_part_roles_for_window(
        {sparse_descriptor},
        "harmonic-bass-role-test");
    assert(!contains_role(
        sparse_roles,
        bass_part,
        musical_part_role::bass_foundation));

    // Repeating a copy of one transition cannot manufacture temporal support.
    bool duplicate_rejected = false;
    try {
        auto duplicated = interactions;
        duplicated.push_back(first_transition);
        (void)infer_harmonic_bass_ownership(
            descriptor_for(bass_part),
            duplicated);
    } catch (const std::invalid_argument&) {
        duplicate_rejected = true;
    }
    assert(duplicate_rejected);

    // Unresolved interaction evidence is coverage loss, not positive ownership.
    auto unresolved = third_transition;
    unresolved.first_time.tick = 3200;
    unresolved.second_time.tick = 3500;
    unresolved.kind = bass_harmony_interaction_kind::unresolved;
    unresolved.bass_identity_grounded = false;
    unresolved.bass_part_id = 0;
    const std::vector<bass_harmony_interaction_hypothesis> with_gap{
        first_transition,
        second_transition,
        third_transition,
        unresolved,
    };
    const auto gap_summary = infer_harmonic_bass_ownership(
        descriptor_for(bass_part),
        with_gap);
    assert(gap_summary.eligible_transitions == 4);
    assert(gap_summary.grounded_transitions == 3);
    assert(gap_summary.confidence < bass_summary.confidence);

    return 0;
}
