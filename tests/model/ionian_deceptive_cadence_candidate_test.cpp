#include "model/ionian_deceptive_cadence_candidate.h"

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
    std::int64_t tick) {
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
    chord.projection.confidence = 0.88;
    return chord;
}

phrase_boundary_evidence evidence(
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
    result.detail = "deceptive cadence regression evidence";
    return result;
}

part_phrase_boundary_hypothesis part(
    node_id part_id,
    std::int64_t tick,
    std::vector<phrase_boundary_evidence> support) {
    part_phrase_boundary_hypothesis result;
    result.part_id = part_id;
    result.boundary = make_phrase_boundary_hypothesis(
        at(tick),
        0.92,
        std::move(support));
    return result;
}

phrase_boundary_consensus closing_boundary(std::int64_t tick) {
    std::vector<part_phrase_boundary_hypothesis> parts;
    parts.push_back(part(11, tick, {
        evidence(
            phrase_boundary_evidence_kind::motif_completion,
            phrase_boundary_evidence_origin::motif_analysis,
            0.90,
            "motif completion"),
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.88,
            "timing support"),
    }));
    parts.push_back(part(12, tick, {
        evidence(
            phrase_boundary_evidence_kind::repeated_motif_alignment,
            phrase_boundary_evidence_origin::motif_analysis,
            0.87,
            "repeated motif completion"),
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.86,
            "second timing support"),
    }));
    return make_phrase_boundary_consensus(std::move(parts));
}

phrase_boundary_consensus timing_only_boundary(std::int64_t tick) {
    std::vector<part_phrase_boundary_hypothesis> parts;
    parts.push_back(part(11, tick, {
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.91,
            "timing only A"),
    }));
    parts.push_back(part(12, tick, {
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.89,
            "timing only B"),
    }));
    return make_phrase_boundary_consensus(std::move(parts));
}

phrase_boundary_consensus circular_boundary(std::int64_t tick) {
    std::vector<part_phrase_boundary_hypothesis> parts;
    for (node_id part_id : {node_id{11}, node_id{12}}) {
        parts.push_back(part(part_id, tick, {
            evidence(
                phrase_boundary_evidence_kind::cadence_or_resolution,
                phrase_boundary_evidence_origin::harmonic_analysis,
                0.95,
                "upstream cadence label"),
            evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                0.90,
                "timing support"),
        }));
    }
    return make_phrase_boundary_consensus(std::move(parts));
}
} // namespace

int main() {
    const auto key = c_ionian_key();
    const auto g_major = triad(7, tertian_triad_quality::major, 100);
    const auto a_minor = triad(9, tertian_triad_quality::minor, 200);

    // Strong case: diatonic V -> VI lands exactly on independently grounded
    // phrase completion. This earns a deceptive-cadence candidate, not a final
    // established cadence class.
    const auto close = closing_boundary(200);
    const auto candidate = infer_ionian_deceptive_cadence_candidate(
        key,
        g_major,
        a_minor,
        close);
    CHECK(candidate.deceptive_cadence_candidate_resolved);
    CHECK(candidate.kind ==
        ionian_deceptive_cadence_candidate_kind::deceptive_cadence_candidate);
    CHECK(candidate.source_degree.has_value() && *candidate.source_degree == 5);
    CHECK(candidate.target_degree.has_value() && *candidate.target_degree == 6);
    CHECK(candidate.five_to_six_morphology);
    CHECK(candidate.diatonic_morphology);
    CHECK(candidate.root_motion_reliable);
    CHECK(candidate.independent_phrase_completion_grounded);
    CHECK(close_enough(
        candidate.confidence,
        ionian_deceptive_cadence_candidate_ceiling));
    CHECK(!candidate.cadence_class_established);
    CHECK(!candidate.roman_numeral_named);

    // Timing punctuation without a completion process does not turn V -> VI
    // into a deceptive cadence candidate.
    const auto timing_only = infer_ionian_deceptive_cadence_candidate(
        key,
        g_major,
        a_minor,
        timing_only_boundary(200));
    CHECK(timing_only.five_to_six_morphology);
    CHECK(!timing_only.independent_phrase_completion_grounded);
    CHECK(!timing_only.deceptive_cadence_candidate_resolved);

    // A cadence-derived phrase label is circular and cannot prove the closure
    // needed by this layer.
    const auto circular = infer_ionian_deceptive_cadence_candidate(
        key,
        g_major,
        a_minor,
        circular_boundary(200));
    CHECK(circular.five_to_six_morphology);
    CHECK(!circular.independent_phrase_completion_grounded);
    CHECK(!circular.deceptive_cadence_candidate_resolved);

    // An ordinary V -> I close is not a deceptive candidate.
    const auto c_major = triad(0, tertian_triad_quality::major, 200);
    const auto authentic = infer_ionian_deceptive_cadence_candidate(
        key,
        g_major,
        c_major,
        close);
    CHECK(!authentic.five_to_six_morphology);
    CHECK(!authentic.deceptive_cadence_candidate_resolved);

    // A chromatic/quality-altered target does not inherit the diatonic VI role.
    const auto a_major = triad(9, tertian_triad_quality::major, 200);
    const auto altered = infer_ionian_deceptive_cadence_candidate(
        key,
        g_major,
        a_major,
        close);
    CHECK(!altered.diatonic_morphology);
    CHECK(!altered.deceptive_cadence_candidate_resolved);

    bool stale_boundary_rejected = false;
    try {
        (void)infer_ionian_deceptive_cadence_candidate(
            key,
            g_major,
            a_minor,
            closing_boundary(201));
    } catch (const std::invalid_argument&) {
        stale_boundary_rejected = true;
    }
    CHECK(stale_boundary_rejected);

    return 0;
}
