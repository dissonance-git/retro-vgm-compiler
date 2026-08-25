#include "model/ionian_cadence_formal_binding.h"

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
    std::vector<std::int64_t> steps,
    std::vector<node_id> parts) {
    if (steps.size() != parts.size())
        throw std::invalid_argument("test triad projection mismatch");
    tertian_triad_hypothesis chord;
    chord.root_pitch_class = positive_mod(root, 12);
    chord.quality = quality;
    chord.inversion = triad_inversion::root_position;
    chord.confidence = 0.88;
    chord.root_ambiguous = false;
    for (std::int64_t offset : triad_template(quality))
        chord.pitch_classes.push_back(positive_mod(root + offset, 12));
    chord.projection.tuning = tuning();
    chord.projection.source_verticality.observation_time = at(tick);
    chord.projection.source_verticality.role = musical_pitch_role::performed;
    chord.projection.source_verticality.part_ids = std::move(parts);
    chord.projection.nearest_steps = std::move(steps);
    chord.projection.confidence = 0.88;
    return chord;
}

voice_leading_hypothesis voices() {
    voice_leading_hypothesis result;
    result.first_time = at(100);
    result.second_time = at(200);
    result.all_correspondence_identity_grounded = true;
    result.confidence = 0.90;
    return result;
}

phrase_boundary_evidence phrase_evidence(
    phrase_boundary_evidence_kind kind,
    phrase_boundary_evidence_origin origin,
    double confidence,
    const char* source) {
    phrase_boundary_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = phrase_boundary_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = source;
    result.detail = "integrated cadence test";
    return result;
}

part_phrase_boundary_hypothesis phrase_part(
    node_id part_id,
    bool completion,
    bool cadence_only = false) {
    std::vector<phrase_boundary_evidence> evidence;
    if (completion) {
        evidence.push_back(phrase_evidence(
            phrase_boundary_evidence_kind::motif_completion,
            phrase_boundary_evidence_origin::motif_analysis,
            0.90,
            "motif completion"));
    } else if (cadence_only) {
        evidence.push_back(phrase_evidence(
            phrase_boundary_evidence_kind::cadence_or_resolution,
            phrase_boundary_evidence_origin::harmonic_analysis,
            0.95,
            "upstream cadence label"));
    }
    evidence.push_back(phrase_evidence(
        phrase_boundary_evidence_kind::temporal_gap,
        phrase_boundary_evidence_origin::performance_timing,
        0.90,
        "timing support"));

    part_phrase_boundary_hypothesis result;
    result.part_id = part_id;
    result.boundary = make_phrase_boundary_hypothesis(
        at(200),
        0.92,
        std::move(evidence));
    return result;
}

phrase_boundary_consensus boundary(bool completion, bool cadence_only = false) {
    std::vector<part_phrase_boundary_hypothesis> parts;
    parts.push_back(phrase_part(101, completion, cadence_only));
    parts.push_back(phrase_part(102, completion, cadence_only));
    return make_phrase_boundary_consensus(std::move(parts));
}

part_role_evidence role_evidence(
    part_role_evidence_kind kind,
    part_role_evidence_origin origin,
    const char* source) {
    part_role_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = part_role_evidence_polarity::supports;
    result.status = evidence_status::hypothesis;
    result.confidence = 0.90;
    result.source = source;
    result.detail = "integrated cadence melodic role";
    return result;
}

musical_part_role_hypothesis melody_role(node_id part_id) {
    std::vector<part_role_evidence> evidence;
    evidence.push_back(role_evidence(
        part_role_evidence_kind::melodic_motif_prominence,
        part_role_evidence_origin::musical_analysis,
        "motif analysis"));
    evidence.push_back(role_evidence(
        part_role_evidence_kind::auditory_salience,
        part_role_evidence_origin::auditory_analysis,
        "auditory analysis"));
    return make_musical_part_role_hypothesis(
        part_id,
        musical_part_role::melodic_foreground,
        time_span{at(150), at(250)},
        0.90,
        std::move(evidence));
}

cadential_arrival_hypothesis arrival_for(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second,
    const phrase_boundary_consensus& phrase_boundary) {
    const auto transition = infer_harmonic_transition(first, second);
    return infer_cadential_arrival(phrase_boundary, transition, voices());
}
} // namespace

int main() {
    constexpr node_id bass_part = 11;
    constexpr node_id inner_part = 12;
    constexpr node_id melody_part = 14;
    constexpr node_id doubling_part = 15;

    const auto key = c_ionian_key();
    const auto g_major = triad(
        7,
        tertian_triad_quality::major,
        100,
        {43, 47, 50, 55},
        {bass_part, inner_part, melody_part, doubling_part});
    const auto c_major = triad(
        0,
        tertian_triad_quality::major,
        200,
        {48, 52, 60, 67},
        {bass_part, inner_part, melody_part, doubling_part});

    const auto closing_boundary = boundary(true);
    const auto authentic_arrival = arrival_for(g_major, c_major, closing_boundary);
    const auto melodic = infer_cadential_melodic_arrival_evidence(
        c_major,
        melody_role(melody_part));
    CHECK(melodic.has_value());

    // PAC-like morphology and independently grounded phrase completion align at
    // the same event. The integrated candidate resolves, but the system still
    // does not claim an established cadence class.
    const auto integrated = infer_ionian_cadence_formal_binding(
        key,
        g_major,
        c_major,
        authentic_arrival,
        closing_boundary,
        voices(),
        std::nullopt,
        melodic);
    CHECK(integrated.morphology_kind ==
        ionian_cadence_candidate_kind::perfect_authentic_cadence_candidate);
    CHECK(integrated.morphology_candidate_resolved);
    CHECK(integrated.formal_closure_candidate_resolved);
    CHECK(integrated.integrated_cadence_candidate_resolved);
    CHECK(close_enough(
        integrated.confidence,
        ionian_integrated_cadence_candidate_ceiling));
    CHECK(!integrated.cadence_class_established);
    CHECK(!integrated.roman_numeral_named);

    // A cadence-derived boundary may still support PAC morphology, but because
    // closure is not independently proven, the integrated candidate stays open.
    const auto circular_boundary = boundary(false, true);
    const auto circular_arrival = arrival_for(g_major, c_major, circular_boundary);
    const auto circular = infer_ionian_cadence_formal_binding(
        key,
        g_major,
        c_major,
        circular_arrival,
        circular_boundary,
        voices(),
        std::nullopt,
        melodic);
    CHECK(circular.morphology_candidate_resolved);
    CHECK(!circular.formal_closure_candidate_resolved);
    CHECK(!circular.integrated_cadence_candidate_resolved);
    CHECK(close_enough(circular.confidence, 0.0));

    // V -> vi can coincide with genuine phrase completion without being
    // relabeled a deceptive cadence. Formal closure and harmonic morphology are
    // independent dimensions.
    const auto a_minor = triad(
        9,
        tertian_triad_quality::minor,
        200,
        {45, 48, 52, 57},
        {bass_part, inner_part, melody_part, doubling_part});
    const auto v_vi_arrival = arrival_for(g_major, a_minor, closing_boundary);
    const auto v_vi = infer_ionian_cadence_formal_binding(
        key,
        g_major,
        a_minor,
        v_vi_arrival,
        closing_boundary,
        voices());
    CHECK(!v_vi.morphology_candidate_resolved);
    CHECK(v_vi.formal_closure_candidate_resolved);
    CHECK(!v_vi.integrated_cadence_candidate_resolved);

    bool stale_boundary_rejected = false;
    try {
        auto stale = closing_boundary;
        stale.representative.tick += 1;
        (void)infer_ionian_cadence_formal_binding(
            key,
            g_major,
            c_major,
            authentic_arrival,
            stale,
            voices(),
            std::nullopt,
            melodic);
    } catch (const std::invalid_argument&) {
        stale_boundary_rejected = true;
    }
    CHECK(stale_boundary_rejected);

    bool forged_confidence_rejected = false;
    try {
        auto forged = closing_boundary;
        forged.confidence -= 0.01;
        (void)infer_ionian_cadence_formal_binding(
            key,
            g_major,
            c_major,
            authentic_arrival,
            forged,
            voices(),
            std::nullopt,
            melodic);
    } catch (const std::invalid_argument&) {
        forged_confidence_rejected = true;
    }
    CHECK(forged_confidence_rejected);

    bool forged_grounding_rejected = false;
    try {
        auto forged = closing_boundary;
        forged.cross_part_grounded = false;
        (void)infer_ionian_cadence_formal_binding(
            key,
            g_major,
            c_major,
            authentic_arrival,
            forged,
            voices(),
            std::nullopt,
            melodic);
    } catch (const std::invalid_argument&) {
        forged_grounding_rejected = true;
    }
    CHECK(forged_grounding_rejected);

    return 0;
}
