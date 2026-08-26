#include "model/structural_composer_grammar_bridge.h"

#include <cmath>
#include <stdexcept>
#include <string>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

structural_grammar_context context(
    const char* soundtrack,
    const char* work,
    const char* source) {
    return {
        soundtrack,
        work,
        composer_representation_kind::synthesis_runtime,
        source,
    };
}

} // namespace

int main() {
    // Harmonic rhythm signatures are tempo-scale invariant because only the
    // normalized shape reaches the blind grammar layer.
    harmonic_rhythm_profile first_rhythm;
    first_rhythm.normalized_change_gaps = {1.0, 2.0, 1.0};
    first_rhythm.confidence = 0.88;
    harmonic_rhythm_profile scaled_rhythm;
    scaled_rhythm.normalized_change_gaps = {1.0, 2.0, 1.0};
    scaled_rhythm.confidence = 0.86;

    const auto rhythm_a = harmonic_rhythm_as_grammar_observation(
        context("soundtrack-a", "work-a", "blind-vgm-a"),
        first_rhythm,
        creative_attribution_role::composer);
    const auto rhythm_b = harmonic_rhythm_as_grammar_observation(
        context("soundtrack-b", "work-b", "blind-vgm-b"),
        scaled_rhythm,
        creative_attribution_role::composer);
    CHECK(rhythm_a.rule_key == rhythm_b.rule_key);
    CHECK(rhythm_a.rule_key == "harmonic_rhythm:1.00,2.00,1.00");
    CHECK(rhythm_a.observation.dimension == composer_grammar_dimension::rhythm);

    // Zero motion is preserved as zero instead of being quantized upward just
    // to satisfy an export/signature helper.
    voice_leading_hypothesis stationary;
    stationary.motions = {
        {60, 60, 0, 1, 1, true},
        {64, 64, 0, 2, 2, true},
    };
    stationary.stationary_voices = 2;
    stationary.identity_preserved_voices = 2;
    stationary.all_correspondence_identity_grounded = true;
    stationary.confidence = 0.90;
    const auto voice_obs = voice_leading_as_grammar_observation(
        context("soundtrack-a", "work-c", "blind-vgm-c"),
        stationary,
        creative_attribution_role::composer);
    CHECK(voice_obs.rule_key.find("motion_per_voice=0.00") != std::string::npos);
    CHECK(voice_obs.observation.dimension ==
        composer_grammar_dimension::counterpoint_voice_leading);

    counterpoint_motion_profile counterpoint;
    counterpoint.similar_motion_count = 0;
    counterpoint.contrary_motion_count = 2;
    counterpoint.oblique_motion_count = 1;
    counterpoint.stationary_motion_count = 0;
    counterpoint.vertical_intervals_comparable = false;
    counterpoint.confidence = 0.85;
    const auto counterpoint_obs = counterpoint_motion_as_grammar_observation(
        context("soundtrack-a", "work-counterpoint", "blind-vgm-counterpoint"),
        counterpoint,
        creative_attribution_role::composer);
    CHECK(counterpoint_obs.rule_key.find("contrary=2") != std::string::npos);
    CHECK(counterpoint_obs.rule_key.find("vertical_interval=unresolved") != std::string::npos);
    CHECK(counterpoint_obs.observation.dimension ==
        composer_grammar_dimension::counterpoint_voice_leading);

    imitative_part_relation_hypothesis imitation;
    imitation.kind = imitative_part_relation_kind::imitation;
    imitation.normalized_onset_lag = 1.5;
    imitation.confidence = 0.90;
    const auto imitation_obs = imitation_as_grammar_observation(
        context("soundtrack-a", "work-imitation", "blind-vgm-imitation"),
        imitation,
        creative_attribution_role::composer);
    CHECK(imitation_obs.rule_key == "imitation:imitation;lag=1.50");
    CHECK(imitation_obs.observation.dimension ==
        composer_grammar_dimension::counterpoint_voice_leading);

    // A creator-facing rule is formed only after two already-extracted blind
    // observations recur across independent soundtracks. The blind bridge itself
    // carries soundtrack/work provenance but no candidate composer identity.
    bass_harmony_interaction_hypothesis first_bass;
    first_bass.kind =
        bass_harmony_interaction_kind::moving_bass_under_retained_upper_material;
    first_bass.retained_upper_pitch_classes = 2;
    first_bass.confidence = 0.82;
    bass_harmony_interaction_hypothesis second_bass = first_bass;
    second_bass.confidence = 0.78;

    const auto bass_a = bass_harmony_as_grammar_observation(
        context("soundtrack-a", "work-d", "blind-vgm-d"),
        first_bass,
        creative_attribution_role::composer);
    const auto bass_b = bass_harmony_as_grammar_observation(
        context("soundtrack-b", "work-e", "blind-spc-e"),
        second_bass,
        creative_attribution_role::composer);
    CHECK(bass_a.rule_key == bass_b.rule_key);
    CHECK(bass_a.observation.soundtrack_id == "soundtrack-a");
    CHECK(bass_b.observation.soundtrack_id == "soundtrack-b");

    const auto creator_rule = make_composer_grammar_rule(
        "candidate-composer",
        creative_attribution_role::composer,
        bass_a.rule_key,
        0.95,
        {bass_a.observation, bass_b.observation});
    CHECK(creator_rule.cross_work_grounded);
    CHECK(creator_rule.cross_soundtrack_grounded);
    CHECK(close_enough(creator_rule.confidence, 0.78));

    // The Sonic 3 VGM surface probe can observe a V->I-shaped transition, but a
    // degree+transition-only functional candidate must not cross into creator
    // grammar. It needs voice-leading or independently grounded phrase arrival.
    ionian_functional_tendency_hypothesis surface_function;
    surface_function.kind =
        ionian_functional_tendency_kind::dominant_resolution_candidate;
    surface_function.source_degree = 5;
    surface_function.target_degree = 1;
    surface_function.source_quality_diatonic = true;
    surface_function.target_quality_diatonic = true;
    surface_function.root_motion_reliable = true;
    surface_function.candidate_resolved = true;
    surface_function.confidence = ionian_function_degree_transition_ceiling;

    bool surface_function_rejected = false;
    try {
        (void)ionian_functional_tendency_as_grammar_observation(
            context("sonic-3-knuckles", "surface-vgm-work", "blind-surface-vgm"),
            surface_function,
            creative_attribution_role::composer);
    } catch (const std::invalid_argument&) {
        surface_function_rejected = true;
    }
    CHECK(surface_function_rejected);

    auto grounded_function = surface_function;
    grounded_function.voice_leading_supplied = true;
    grounded_function.voice_leading_identity_grounded = true;
    grounded_function.confidence = ionian_function_identity_voice_ceiling;
    const auto function_obs = ionian_functional_tendency_as_grammar_observation(
        context("soundtrack-a", "functional-work-a", "blind-structural-a"),
        grounded_function,
        creative_attribution_role::composer);
    CHECK(function_obs.rule_key.find("dominant_resolution_candidate") != std::string::npos);
    CHECK(function_obs.rule_key.find("degree=5>1") != std::string::npos);
    CHECK(function_obs.observation.dimension == composer_grammar_dimension::bass_harmony);
    CHECK(close_enough(function_obs.observation.confidence, ionian_function_identity_voice_ceiling));

    // Cadence grammar requires a resolved cadence candidate *and* cross-part
    // phrase grounding. Merely seeing the V->I surface shape is insufficient.
    ionian_cadence_class_hypothesis weak_cadence;
    weak_cadence.kind = ionian_cadence_candidate_kind::authentic_cadence_candidate;
    weak_cadence.functional_tendency =
        ionian_functional_tendency_kind::dominant_resolution_candidate;
    weak_cadence.source_degree = 5;
    weak_cadence.target_degree = 1;
    weak_cadence.cadence_candidate_resolved = true;
    weak_cadence.confidence = 0.69;

    bool weak_cadence_rejected = false;
    try {
        (void)ionian_cadence_as_grammar_observation(
            context("sonic-3-knuckles", "surface-vgm-work", "blind-surface-vgm"),
            weak_cadence,
            creative_attribution_role::composer);
    } catch (const std::invalid_argument&) {
        weak_cadence_rejected = true;
    }
    CHECK(weak_cadence_rejected);

    auto pac = weak_cadence;
    pac.kind = ionian_cadence_candidate_kind::perfect_authentic_cadence_candidate;
    pac.phrase_cross_part_grounded = true;
    pac.source_root_position = true;
    pac.target_root_position = true;
    pac.final_soprano_observed = true;
    pac.final_soprano_tonic = true;
    pac.confidence = 0.82;

    const auto cadence_a = ionian_cadence_as_grammar_observation(
        context("soundtrack-a", "cadence-work-a", "blind-structural-a"),
        pac,
        creative_attribution_role::composer);
    auto pac_second = pac;
    pac_second.confidence = 0.80;
    const auto cadence_b = ionian_cadence_as_grammar_observation(
        context("soundtrack-b", "cadence-work-b", "blind-structural-b"),
        pac_second,
        creative_attribution_role::composer);
    CHECK(cadence_a.rule_key == cadence_b.rule_key);
    CHECK(cadence_a.rule_key.find("perfect_authentic_cadence_candidate") != std::string::npos);
    CHECK(cadence_a.observation.dimension == composer_grammar_dimension::phrase_form);

    // Two independent works from Sonic 3 alone can establish recurrence within
    // one soundtrack, but the composer-grammar kernel keeps the soundtrack-local
    // ceiling. Sonic 3 cannot teach its own attribution answer.
    const auto cadence_same_soundtrack_b = ionian_cadence_as_grammar_observation(
        context("soundtrack-a", "cadence-work-b", "blind-structural-b"),
        pac_second,
        creative_attribution_role::composer);
    const auto soundtrack_local_rule = make_composer_grammar_rule(
        "candidate-composer",
        creative_attribution_role::composer,
        cadence_a.rule_key,
        0.95,
        {cadence_a.observation, cadence_same_soundtrack_b.observation});
    CHECK(soundtrack_local_rule.cross_work_grounded);
    CHECK(!soundtrack_local_rule.cross_soundtrack_grounded);
    CHECK(close_enough(
        soundtrack_local_rule.confidence,
        composer_grammar_single_soundtrack_ceiling));

    const auto cadence_cross_soundtrack_rule = make_composer_grammar_rule(
        "candidate-composer",
        creative_attribution_role::composer,
        cadence_a.rule_key,
        0.95,
        {cadence_a.observation, cadence_b.observation});
    CHECK(cadence_cross_soundtrack_rule.cross_work_grounded);
    CHECK(cadence_cross_soundtrack_rule.cross_soundtrack_grounded);
    CHECK(close_enough(cadence_cross_soundtrack_rule.confidence, 0.80));

    // Tonal-region grammar is even stricter: a bare contrasting-center reading
    // is not enough. Only independently grounded tonicization/modulation/return
    // relations are allowed through the bridge.
    tonal_region_relation_hypothesis contrast;
    contrast.kind = tonal_region_relation_kind::contrasting_center;
    contrast.topology = tonal_region_topology::sequential;
    contrast.center_distance_octaves = 7.0 / 12.0;
    contrast.target_center_cross_origin_grounded = true;
    contrast.independent_support_groups = 3;
    contrast.independent_support_origins = 3;
    contrast.confidence = tonal_region_contrast_ceiling;

    bool contrast_rejected = false;
    try {
        (void)tonal_region_as_grammar_observation(
            context("sonic-3-knuckles", "surface-region", "blind-surface-vgm"),
            contrast,
            creative_attribution_role::composer);
    } catch (const std::invalid_argument&) {
        contrast_rejected = true;
    }
    CHECK(contrast_rejected);

    tonal_region_relation_hypothesis modulation = contrast;
    modulation.kind = tonal_region_relation_kind::modulation_candidate;
    modulation.center_distance_octaves = 5.0 / 12.0;
    modulation.target_key_class_resolved = true;
    modulation.confidence = 0.82;
    const auto modulation_obs = tonal_region_as_grammar_observation(
        context("soundtrack-a", "modulation-work", "blind-structural-modulation"),
        modulation,
        creative_attribution_role::composer);
    CHECK(modulation_obs.rule_key.find("modulation_candidate") != std::string::npos);
    CHECK(modulation_obs.rule_key.find("topology=sequential") != std::string::npos);
    CHECK(modulation_obs.rule_key.find("center_distance_semitones=5.00") != std::string::npos);
    CHECK(modulation_obs.rule_key.find("soundtrack-a") == std::string::npos);
    CHECK(modulation_obs.observation.dimension == composer_grammar_dimension::phrase_form);

    // The rule key is transposition-invariant: absolute source/target centers are
    // intentionally absent. Only the relation and center distance survive.
    tonal_region_relation_hypothesis transposed_modulation = modulation;
    const auto transposed_modulation_obs = tonal_region_as_grammar_observation(
        context("soundtrack-b", "modulation-work-b", "blind-structural-modulation-b"),
        transposed_modulation,
        creative_attribution_role::composer);
    CHECK(modulation_obs.rule_key == transposed_modulation_obs.rule_key);

    return 0;
}
