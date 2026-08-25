#include "model/ionian_cadence_class_hypothesis.h"

#include <cmath>
#include <stdexcept>
#include <utility>
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
    std::vector<node_id> part_ids,
    triad_inversion inversion = triad_inversion::root_position,
    double confidence = 0.88) {
    if (projected_steps.size() != part_ids.size())
        throw std::invalid_argument("test triad requires one part id per projected step");

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
    chord.projection.source_verticality.part_ids = std::move(part_ids);
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

part_phrase_boundary_hypothesis phrase_part(
    node_id part_id,
    std::int64_t tick,
    double confidence = 0.90) {
    part_phrase_boundary_hypothesis result;
    result.part_id = part_id;
    result.boundary.boundary = at(tick);
    result.boundary.confidence = confidence;
    return result;
}

cadential_arrival_hypothesis arrival_between(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second,
    bool cross_part = true,
    bool voice_grounded = true) {
    const auto transition = infer_harmonic_transition(first, second);
    const std::int64_t first_tick =
        first.projection.source_verticality.observation_time.tick;
    const std::int64_t second_tick =
        second.projection.source_verticality.observation_time.tick;

    std::vector<part_phrase_boundary_hypothesis> parts;
    parts.push_back(phrase_part(101, second_tick));
    if (cross_part)
        parts.push_back(phrase_part(102, second_tick));
    const auto boundary = make_phrase_boundary_consensus(std::move(parts));

    if (voice_grounded) {
        const auto voices = grounded_voices(first_tick, second_tick);
        return infer_cadential_arrival(boundary, transition, voices);
    }
    return infer_cadential_arrival(boundary, transition);
}

part_role_evidence role_evidence(
    part_role_evidence_kind kind,
    part_role_evidence_origin origin,
    double confidence,
    const char* source) {
    part_role_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = part_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = source;
    result.detail = "cadence test role evidence";
    return result;
}

musical_part_role_hypothesis melodic_role(
    node_id part_id,
    std::int64_t start_tick = 150,
    std::int64_t end_tick = 250) {
    std::vector<part_role_evidence> evidence;
    evidence.push_back(role_evidence(
        part_role_evidence_kind::melodic_motif_prominence,
        part_role_evidence_origin::musical_analysis,
        0.90,
        "test motif analysis"));
    evidence.push_back(role_evidence(
        part_role_evidence_kind::auditory_salience,
        part_role_evidence_origin::auditory_analysis,
        0.90,
        "test auditory analysis"));
    return make_musical_part_role_hypothesis(
        part_id,
        musical_part_role::melodic_foreground,
        time_span{at(start_tick), at(end_tick)},
        0.90,
        std::move(evidence));
}

musical_part_role_hypothesis realization_only_melodic_role(node_id part_id) {
    std::vector<part_role_evidence> evidence;
    evidence.push_back(role_evidence(
        part_role_evidence_kind::register_position,
        part_role_evidence_origin::musical_analysis,
        0.95,
        "test register shortcut"));
    return make_musical_part_role_hypothesis(
        part_id,
        musical_part_role::melodic_foreground,
        time_span{at(150), at(250)},
        0.95,
        std::move(evidence));
}
} // namespace

int main() {
    constexpr node_id bass_part = 11;
    constexpr node_id inner_part = 12;
    constexpr node_id upper_part = 13;
    constexpr node_id melody_part = 14;
    constexpr node_id doubling_part = 15;

    const auto key = c_ionian_key();
    const auto g_major = triad(
        7,
        tertian_triad_quality::major,
        100,
        {43, 47, 50, 55},
        {bass_part, inner_part, upper_part, doubling_part});
    const auto voices = grounded_voices();

    // Adversarial PAC: the actual melodic foreground lands on tonic C at step
    // 60, while a separate doubling part sits higher on G at step 67. Highest
    // sounding pitch therefore disagrees with the grounded melodic identity.
    const auto c_major_melody_c_high_g = triad(
        0,
        tertian_triad_quality::major,
        200,
        {48, 52, 60, 67},
        {bass_part, inner_part, melody_part, doubling_part});
    const auto v_i_arrival = arrival_between(g_major, c_major_melody_c_high_g);
    const auto melody_c = infer_cadential_melodic_arrival_evidence(
        c_major_melody_c_high_g,
        melodic_role(melody_part));
    CHECK(melody_c.has_value());
    CHECK(melody_c->part_id == melody_part);
    CHECK(melody_c->projected_step == 60);
    CHECK(melody_c->pitch_class == 0);

    const auto pac = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_major_melody_c_high_g,
        v_i_arrival,
        voices,
        std::nullopt,
        melody_c);
    CHECK(pac.cadence_candidate_resolved);
    CHECK(pac.kind == ionian_cadence_candidate_kind::perfect_authentic_cadence_candidate);
    CHECK(pac.source_degree.has_value() && *pac.source_degree == 5);
    CHECK(pac.target_degree.has_value() && *pac.target_degree == 1);
    CHECK(pac.source_root_position);
    CHECK(pac.target_root_position);
    CHECK(pac.final_melodic_arrival_grounded);
    CHECK(pac.final_melodic_part_id == melody_part);
    CHECK(pac.final_melodic_pitch_class.has_value() && *pac.final_melodic_pitch_class == 0);
    CHECK(pac.final_melodic_tonic);
    CHECK(close_enough(pac.confidence, ionian_specific_authentic_cadence_ceiling));
    CHECK(!pac.cadence_class_established);
    CHECK(!pac.roman_numeral_named);

    // Mirror adversary: the highest pitch is tonic C, but the grounded melody
    // ends on E. The old top-note shortcut would call this PAC; persistent-part
    // evidence correctly refines it only to an IAC candidate.
    const auto c_major_melody_e_high_c = triad(
        0,
        tertian_triad_quality::major,
        200,
        {48, 52, 55, 72},
        {bass_part, melody_part, inner_part, doubling_part});
    const auto v_i_e_arrival = arrival_between(g_major, c_major_melody_e_high_c);
    const auto melody_e = infer_cadential_melodic_arrival_evidence(
        c_major_melody_e_high_c,
        melodic_role(melody_part));
    CHECK(melody_e.has_value());
    CHECK(melody_e->pitch_class == 4);

    const auto iac = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_major_melody_e_high_c,
        v_i_e_arrival,
        voices,
        std::nullopt,
        melody_e);
    CHECK(iac.cadence_candidate_resolved);
    CHECK(iac.kind == ionian_cadence_candidate_kind::imperfect_authentic_cadence_candidate);
    CHECK(iac.final_melodic_arrival_grounded);
    CHECK(!iac.final_melodic_tonic);

    // Without a grounded melodic role, even an obvious high tonic cannot be
    // used to fabricate PAC/IAC specificity. The result stays generic.
    const auto generic = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_major_melody_e_high_c,
        v_i_e_arrival,
        voices);
    CHECK(generic.cadence_candidate_resolved);
    CHECK(generic.kind == ionian_cadence_candidate_kind::authentic_cadence_candidate);
    CHECK(!generic.final_melodic_arrival_grounded);
    CHECK(!generic.final_melodic_pitch_class.has_value());

    // Register alone is realization evidence, not enough to establish melody.
    const auto weak_role = realization_only_melodic_role(doubling_part);
    CHECK(weak_role.realization_only);
    CHECK(weak_role.confidence == part_role_realization_only_ceiling);
    const auto weak_melodic = infer_cadential_melodic_arrival_evidence(
        c_major_melody_e_high_c,
        weak_role);
    CHECK(!weak_melodic.has_value());

    // A valid melodic role outside the arrival's active span is also absent
    // evidence, not a negative measurement.
    const auto inactive_role = melodic_role(melody_part, 50, 150);
    const auto inactive_melodic = infer_cadential_melodic_arrival_evidence(
        c_major_melody_e_high_c,
        inactive_role);
    CHECK(!inactive_melodic.has_value());

    // An inverted target remains IAC even when the grounded melody lands tonic.
    const auto c_first_inversion = triad(
        0,
        tertian_triad_quality::major,
        200,
        {52, 55, 60},
        {bass_part, inner_part, melody_part},
        triad_inversion::first);
    const auto inverted_arrival = arrival_between(g_major, c_first_inversion);
    const auto inverted_melody = infer_cadential_melodic_arrival_evidence(
        c_first_inversion,
        melodic_role(melody_part));
    CHECK(inverted_melody.has_value());
    const auto inverted = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_first_inversion,
        inverted_arrival,
        voices,
        std::nullopt,
        inverted_melody);
    CHECK(inverted.kind == ionian_cadence_candidate_kind::imperfect_authentic_cadence_candidate);
    CHECK(!inverted.target_root_position);
    CHECK(inverted.final_melodic_tonic);

    // A forged melodic witness that names the right part but the wrong exact
    // projected step is rejected instead of being laundered through pitch class.
    bool forged_melody_rejected = false;
    try {
        auto forged = *melody_c;
        forged.projected_step += 12;
        (void)infer_ionian_cadence_class_hypothesis(
            key,
            g_major,
            c_major_melody_c_high_g,
            v_i_arrival,
            voices,
            std::nullopt,
            forged);
    } catch (const std::invalid_argument&) {
        forged_melody_rejected = true;
    }
    CHECK(forged_melody_rejected);

    // vii-diminished -> I has strong resolution pressure, but authentic cadence
    // terminology is reserved for V -> I here.
    const auto b_dim = triad(
        11,
        tertian_triad_quality::diminished,
        100,
        {47, 50, 53, 59},
        {bass_part, inner_part, upper_part, doubling_part});
    const auto vii_i_arrival = arrival_between(b_dim, c_major_melody_c_high_g);
    const auto leading_tone = infer_ionian_cadence_class_hypothesis(
        key,
        b_dim,
        c_major_melody_c_high_g,
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
        {50, 53, 57},
        {bass_part, inner_part, melody_part});
    const auto g_at_arrival = triad(
        7,
        tertian_triad_quality::major,
        200,
        {43, 47, 50, 55},
        {bass_part, inner_part, melody_part, doubling_part});
    const auto ii_v_arrival = arrival_between(d_minor, g_at_arrival);
    const auto ii_v = infer_ionian_cadence_class_hypothesis(
        key,
        d_minor,
        g_at_arrival,
        ii_v_arrival,
        voices);
    CHECK(ii_v.cadence_candidate_resolved);
    CHECK(ii_v.kind == ionian_cadence_candidate_kind::predominant_half_cadence_candidate);
    CHECK(close_enough(ii_v.confidence, ionian_predominant_half_cadence_ceiling));

    // Phrase grounding is essential. A numerically strong single-part arrival
    // cannot create a global cadence class.
    const auto local_arrival = arrival_between(
        g_major,
        c_major_melody_c_high_g,
        false,
        true);
    const auto local_only = infer_ionian_cadence_class_hypothesis(
        key,
        g_major,
        c_major_melody_c_high_g,
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
        {45, 48, 52, 57},
        {bass_part, inner_part, melody_part, doubling_part});
    const auto v_vi_arrival = arrival_between(g_major, a_minor);
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
