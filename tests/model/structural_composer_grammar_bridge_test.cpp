#include "model/structural_composer_grammar_bridge.h"

#include <cmath>
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

    return 0;
}
