#include "model/ionian_cadence_class_hypothesis.h"

#include <cmath>
#include <stdexcept>
#include <vector>

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
    std::vector<std::int64_t> projected_steps,
    triad_inversion inversion = triad_inversion::root_position,
    double confidence = 0.88) {
    tertian_triad_hypothesis chord;
    chord.root_pitch_class = positive_mod(root, 12);
    chord.quality = quality;
    chord.inversion = inversion;
    chord.confidence = confidence;
    chord.root_ambiguous = false;
    for (std::int64_t offset : triad_template(quality))
        chord.pitch_classes.push_back(positive_mod(root + offset, 12));
    chord.projection.tuning = tuning();
    chord.projection.source_verticality.observation_time = at(tick);
    chord.projection.source_verticality.role = musical_pitch_role::performed;
    chord.projection.nearest_steps = std::move(projected_steps);
    chord.projection.confidence = confidence;
    return chord;
}

voice_leading_hypothesis grounded_voices(
    std::int64_t first_tick = 100,
    std::int64_t second_tick = 200) {
    voice_leading_hypothesis result;
    result.first_time = at(first_tick);
    result.second_time = at(second_tick);
    result.all_correspondence_identity_grounded = true;
    result.confidence = 0.90;
    return result;
}

cadential_arrival_hypothesis arrival(
    std::int64_t root_motion,
    std::int64_t interval_class,
    bool cross_part = true,
    bool voice_grounded = true,
    std::int64_t tick = 200) {
    cadential_arrival_hypothesis result;
    result.arrival_time = at(tick);
    result.root_motion_semitones = root_motion;
    result.root_interval_class = interval_class;
    result.harmonic_root_motion_reliable = true;
    result.cross_part_phrase_grounded = cross_part;
    result.voice_leading_grounded = voice_grounded;
    result.voice_leading_confidence = voice_grounded ? 0.90 : 0.0;
    result.confidence = 0.90;
    return result;
}
} // namespace

int main() {
    const auto key = c_ionian_key();
    const auto g_major = triad(
        7,
        tertian_triad_quality::major,
        100,
        {43, 47, 50, 55});
    const auto c_major_soprano_c = triad(
        0,
        tertian_triad_quality::major,
        200,
        {48, 52, 55, 60});
    const auto voices = grounded_voices();
    const auto v_i_arrival = arrival(5, 5);

    // Root-position V -> I, cross-part phrase arrival, and a final tonic in the
    // soprano earn a PAC candidate inside the narrow Ionian theory scope.
    const auto pac = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_major_soprano_c,
        v_i_arrival,
        voices);
    CHECK(pac.cadence_candidate_resolved);
    CHECK(pac.kind == ionian_cadence_candidate_kind::perfect_authentic_cadence_candidate);
    CHECK(pac.source_degree.has_value() && *pac.source_degree == 5);
    CHECK(pac.target_degree.has_value() && *pac.target_degree == 1);
    CHECK(pac.source_root_position);
    CHECK(pac.target_root_position);
    CHECK(pac.final_soprano_observed);
    CHECK(pac.final_soprano_tonic);
    CHECK(close_enough(pac.confidence, ionian_specific_authentic_cadence_ceiling));
    CHECK(!pac.cadence_class_established);
    CHECK(!pac.roman_numeral_named);

    // The same V -> I progression ending with scale degree 3 in the soprano is
    // an IAC candidate, not a PAC.
    const auto c_major_soprano_e = triad(
        0,
        tertian_triad_quality::major,
        200,
        {48, 55, 64});
    const auto iac = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_major_soprano_e,
        v_i_arrival,
        voices);
    CHECK(iac.cadence_candidate_resolved);
    CHECK(iac.kind == ionian_cadence_candidate_kind::imperfect_authentic_cadence_candidate);
    CHECK(iac.final_soprano_observed);
    CHECK(!iac.final_soprano_tonic);

    // Without final voicing evidence the engine may recognize an authentic
    // cadence candidate, but must not invent PAC versus IAC.
    const auto c_major_no_voicing = triad(
        0,
        tertian_triad_quality::major,
        200,
        {});
    const auto generic = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_major_no_voicing,
        v_i_arrival,
        voices);
    CHECK(generic.cadence_candidate_resolved);
    CHECK(generic.kind == ionian_cadence_candidate_kind::authentic_cadence_candidate);
    CHECK(!generic.final_soprano_observed);

    // An inverted arrival cannot satisfy PAC even if the soprano is tonic.
    const auto c_first_inversion = triad(
        0,
        tertian_triad_quality::major,
        200,
        {52, 55, 60},
        triad_inversion::first);
    const auto inverted = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_first_inversion,
        v_i_arrival,
        voices);
    CHECK(inverted.kind == ionian_cadence_candidate_kind::imperfect_authentic_cadence_candidate);
    CHECK(!inverted.target_root_position);
    CHECK(inverted.final_soprano_tonic);

    // vii-diminished -> I has strong resolution pressure, but authentic cadence
    // terminology is reserved for V -> I here.
    const auto b_dim = triad(
        11,
        tertian_triad_quality::diminished,
        100,
        {47, 50, 53, 59});
    const auto vii_i_arrival = arrival(1, 1);
    const auto leading_tone = infer_ionian_cadence_class_hypothesis(
        key,
        b_dim,
        c_major_soprano_c,
        vii_i_arrival,
        voices);
    CHECK(leading_tone.cadence_candidate_resolved);
    CHECK(leading_tone.kind == ionian_cadence_candidate_kind::leading_tone_resolution_candidate);
    CHECK(close_enough(leading_tone.confidence, ionian_leading_tone_resolution_ceiling));

    // A grounded ii -> V phrase ending is a deliberately narrow half-cadence
    // candidate. The model is not claiming all half-cadence approach patterns.
    const auto d_minor = triad(
        2,
        tertian_triad_quality::minor,
        100,
        {50, 53, 57});
    const auto g_at_arrival = triad(
        7,
        tertian_triad_quality::major,
        200,
        {43, 47, 50, 55});
    const auto ii_v = infer_ionian_cadence_class_hypothesis(
        key,
        d_minor,
        g_at_arrival,
        v_i_arrival,
        voices);
    CHECK(ii_v.cadence_candidate_resolved);
    CHECK(ii_v.kind == ionian_cadence_candidate_kind::predominant_half_cadence_candidate);
    CHECK(close_enough(ii_v.confidence, ionian_predominant_half_cadence_ceiling));

    // Phrase grounding is essential. A numerically strong single-part arrival
    // cannot create a global cadence class.
    const auto local_arrival = arrival(5, 5, false, true);
    const auto local_only = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_major_soprano_c,
        local_arrival,
        voices);
    CHECK(!local_only.cadence_candidate_resolved);
    CHECK(local_only.kind == ionian_cadence_candidate_kind::unresolved);
    CHECK(close_enough(local_only.confidence, 0.0));

    // V -> vi is not force-fit into an authentic or deceptive cadence class yet.
    // Deceptive cadence requires its own evidence rule rather than a name inferred
    // from one surprising target chord.
    const auto a_minor = triad(
        9,
        tertian_triad_quality::minor,
        200,
        {45, 48, 52, 57});
    const auto v_vi_arrival = arrival(2, 2);
    const auto deceptive_unresolved = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        a_minor,
        v_vi_arrival,
        voices);
    CHECK(!deceptive_unresolved.cadence_candidate_resolved);
    CHECK(deceptive_unresolved.kind == ionian_cadence_candidate_kind::unresolved);

    return 0;
}
