#include "model/ionian_functional_tendency_hypothesis.h"

#include <cmath>
#include <stdexcept>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

equal_temperament_model tuning() {
    equal_temperament_model result;
    result.divisions_per_octave = 12;
    result.reference_frequency_hz = 440.0;
    result.reference_step = 69;
    result.confidence = 1.0;
    result.source = "test 12-TET";
    return result;
}

tonal_key_class_hypothesis c_ionian_key() {
    tonal_key_class_hypothesis key;
    key.region = time_span{at(0), at(1000)};
    key.tuning = tuning();
    key.pitch_role = musical_pitch_role::performed;
    key.center_octave_class = equal_temperament_step_octave_class(key.tuning, 0);
    key.center_pitch_class = 0;
    key.mode = diatonic_mode::ionian;
    key.collection_scope = pitch_class_collection_scope::structural_hypothesis;
    key.center_cross_origin_grounded = true;
    key.key_class_resolved = true;
    key.confidence = 0.82;
    return key;
}

tertian_triad_hypothesis triad(
    std::int64_t root,
    tertian_triad_quality quality,
    std::int64_t tick,
    double confidence = 0.88) {
    tertian_triad_hypothesis chord;
    chord.root_pitch_class = positive_mod(root, 12);
    chord.quality = quality;
    chord.inversion = triad_inversion::root_position;
    chord.confidence = confidence;
    chord.root_ambiguous = false;
    for (std::int64_t offset : triad_template(quality))
        chord.pitch_classes.push_back(positive_mod(root + offset, 12));
    chord.projection.tuning = tuning();
    chord.projection.source_verticality.observation_time = at(tick);
    chord.projection.source_verticality.role = musical_pitch_role::performed;
    chord.projection.confidence = confidence;
    return chord;
}

voice_leading_hypothesis voices(
    bool identity_grounded,
    double confidence = 0.90,
    std::int64_t first_tick = 100,
    std::int64_t second_tick = 200) {
    voice_leading_hypothesis result;
    result.first_time = at(first_tick);
    result.second_time = at(second_tick);
    result.all_correspondence_identity_grounded = identity_grounded;
    result.confidence = confidence;
    return result;
}

bass_harmony_interaction_hypothesis bass(
    double confidence = 0.86,
    std::int64_t first_tick = 100,
    std::int64_t second_tick = 200) {
    bass_harmony_interaction_hypothesis result;
    result.first_time = at(first_tick);
    result.second_time = at(second_tick);
    result.kind = bass_harmony_interaction_kind::generic_harmonic_change;
    result.bass_part_id = 1;
    result.bass_identity_grounded = true;
    result.confidence = confidence;
    return result;
}

cadential_arrival_hypothesis dominant_arrival(
    double confidence,
    bool cross_part,
    bool voice_grounded,
    std::int64_t arrival_tick = 200) {
    cadential_arrival_hypothesis result;
    result.arrival_time = at(arrival_tick);
    result.root_motion_semitones = 5;
    result.root_interval_class = 5;
    result.harmonic_root_motion_reliable = true;
    result.cross_part_phrase_grounded = cross_part;
    result.voice_leading_grounded = voice_grounded;
    result.voice_leading_confidence = voice_grounded ? 0.90 : 0.0;
    result.confidence = confidence;
    return result;
}
} // namespace

int main() {
    const auto key = c_ionian_key();
    const auto g_major = triad(7, tertian_triad_quality::major, 100);
    const auto c_major = triad(0, tertian_triad_quality::major, 200);

    // Degree structure + reliable harmonic transition identifies only a
    // low-ceiling dominant-resolution tendency candidate.
    const auto structural = infer_ionian_functional_tendency(
        key,
        g_major,
        c_major);
    CHECK(structural.candidate_resolved);
    CHECK(structural.kind ==
        ionian_functional_tendency_kind::dominant_resolution_candidate);
    CHECK(structural.source_degree.has_value());
    CHECK(*structural.source_degree == 5);
    CHECK(structural.target_degree.has_value());
    CHECK(*structural.target_degree == 1);
    CHECK(close_enough(structural.confidence, ionian_function_degree_transition_ceiling));
    CHECK(!structural.tonal_function_established);
    CHECK(!structural.roman_numeral_named);

    // Identity-grounded voice leading raises the allowed evidence tier, but it
    // still does not establish function or Roman numeral by itself.
    const auto grounded_voices = voices(true);
    const auto with_voices = infer_ionian_functional_tendency(
        key,
        g_major,
        c_major,
        grounded_voices);
    CHECK(with_voices.voice_leading_supplied);
    CHECK(with_voices.voice_leading_identity_grounded);
    CHECK(close_enough(with_voices.confidence, ionian_function_identity_voice_ceiling));
    CHECK(!with_voices.tonal_function_established);

    // Bass is downstream of the voice-leading witness. It may constrain the
    // candidate but cannot create a new independent confidence tier.
    const auto weak_bass = bass(0.68);
    const auto with_bass = infer_ionian_functional_tendency(
        key,
        g_major,
        c_major,
        grounded_voices,
        weak_bass);
    CHECK(with_bass.bass_evidence_supplied);
    CHECK(with_bass.bass_identity_grounded);
    CHECK(close_enough(with_bass.confidence, 0.68));

    // Cross-part phrase arrival plus identity-grounded voice leading can reach
    // the stronger 0.82 tendency ceiling, bounded here by the key itself.
    const auto strong_arrival = dominant_arrival(0.90, true, true);
    const auto phrase_grounded = infer_ionian_functional_tendency(
        key,
        g_major,
        c_major,
        grounded_voices,
        std::nullopt,
        strong_arrival);
    CHECK(phrase_grounded.phrase_arrival_supplied);
    CHECK(phrase_grounded.phrase_arrival_cross_part_grounded);
    CHECK(close_enough(phrase_grounded.confidence, ionian_function_phrase_arrival_ceiling));
    CHECK(!phrase_grounded.tonal_function_established);
    CHECK(!phrase_grounded.roman_numeral_named);

    // A type-correct but overconfident arrival without voice grounding is
    // re-capped to the cadence layer's phrase+harmony ceiling. Cross-part=true
    // does not let a fabricated 0.99 value jump directly to 0.82.
    const auto raw_arrival = dominant_arrival(0.99, true, false);
    const auto recapped_arrival = infer_ionian_functional_tendency(
        key,
        g_major,
        c_major,
        std::nullopt,
        std::nullopt,
        raw_arrival);
    CHECK(close_enough(recapped_arrival.confidence, phrase_harmony_arrival_ceiling));

    // vii-diminished -> I is also a narrow dominant-resolution candidate under
    // the explicit Ionian theory scope.
    const auto b_dim = triad(11, tertian_triad_quality::diminished, 100);
    const auto leading_tone_resolution = infer_ionian_functional_tendency(
        key,
        b_dim,
        c_major);
    CHECK(leading_tone_resolution.candidate_resolved);
    CHECK(leading_tone_resolution.kind ==
        ionian_functional_tendency_kind::dominant_resolution_candidate);
    CHECK(*leading_tone_resolution.source_degree == 7);

    // Diatonic ii-minor -> V-major is a predominant-progression candidate.
    const auto d_minor = triad(2, tertian_triad_quality::minor, 100);
    const auto g_later = triad(7, tertian_triad_quality::major, 200);
    const auto predominant = infer_ionian_functional_tendency(
        key,
        d_minor,
        g_later);
    CHECK(predominant.candidate_resolved);
    CHECK(predominant.kind ==
        ionian_functional_tendency_kind::predominant_progression_candidate);
    CHECK(*predominant.source_degree == 2);
    CHECK(*predominant.target_degree == 5);

    // D major in C has a diatonic root but altered quality. It remains unresolved
    // here rather than being mislabeled predominant or silently promoted to V/V.
    const auto d_major = triad(2, tertian_triad_quality::major, 100);
    const auto altered_two = infer_ionian_functional_tendency(
        key,
        d_major,
        g_later);
    CHECK(!altered_two.candidate_resolved);
    CHECK(altered_two.kind == ionian_functional_tendency_kind::unresolved);
    CHECK(close_enough(altered_two.confidence, 0.0));
    CHECK(!altered_two.tonal_function_established);
    CHECK(!altered_two.roman_numeral_named);

    bool non_ionian_rejected = false;
    try {
        auto dorian = key;
        dorian.mode = diatonic_mode::dorian;
        (void)infer_ionian_functional_tendency(dorian, g_major, c_major);
    } catch (const std::invalid_argument&) {
        non_ionian_rejected = true;
    }
    CHECK(non_ionian_rejected);

    bool stale_voice_rejected = false;
    try {
        const auto stale = voices(true, 0.99, 90, 190);
        (void)infer_ionian_functional_tendency(
            key,
            g_major,
            c_major,
            stale);
    } catch (const std::invalid_argument&) {
        stale_voice_rejected = true;
    }
    CHECK(stale_voice_rejected);

    bool bass_without_voice_rejected = false;
    try {
        (void)infer_ionian_functional_tendency(
            key,
            g_major,
            c_major,
            std::nullopt,
            bass());
    } catch (const std::invalid_argument&) {
        bass_without_voice_rejected = true;
    }
    CHECK(bass_without_voice_rejected);

    bool mismatched_arrival_rejected = false;
    try {
        const auto wrong_arrival = dominant_arrival(0.82, true, false, 210);
        (void)infer_ionian_functional_tendency(
            key,
            g_major,
            c_major,
            std::nullopt,
            std::nullopt,
            wrong_arrival);
    } catch (const std::invalid_argument&) {
        mismatched_arrival_rejected = true;
    }
    CHECK(mismatched_arrival_rejected);

    bool missing_voice_witness_rejected = false;
    try {
        const auto voice_claiming_arrival = dominant_arrival(0.82, true, true);
        (void)infer_ionian_functional_tendency(
            key,
            g_major,
            c_major,
            std::nullopt,
            std::nullopt,
            voice_claiming_arrival);
    } catch (const std::invalid_argument&) {
        missing_voice_witness_rejected = true;
    }
    CHECK(missing_voice_witness_rejected);

    return 0;
}
