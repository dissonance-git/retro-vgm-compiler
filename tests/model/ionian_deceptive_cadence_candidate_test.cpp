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

voice_leading_hypothesis voices(std::int64_t first_tick, std::int64_t second_tick) {
    voice_leading_hypothesis result;
    result.first_time = at(first_tick);
    result.second_time = at(second_tick);
    result.all_correspondence_identity_grounded = true;
    result.confidence = 0.90;
    return result;
}

phrase_boundary_evidence evidence(
    phrase_boundary_evidence_kind kind,
    phrase_boundary_evidence_origin origin,
    phrase_boundary_evidence_polarity polarity,
    double confidence,
    const char* source) {
    phrase_boundary_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = polarity;
    result.status = evidence_status::hypothesis;
    result.confidence = confidence;
    result.source = source;
    result.detail = "deceptive cadence candidate regression";
    return result;
}

part_phrase_boundary_hypothesis diversion_part(
    node_id part_id,
    std::vector<phrase_boundary_evidence> items) {
    part_phrase_boundary_hypothesis result;
    result.part_id = part_id;
    result.boundary = make_phrase_boundary_hypothesis(
        at(200),
        0.92,
        std::move(items));
    return result;
}

std::vector<part_phrase_boundary_hypothesis> strong_deviation_conflict() {
    std::vector<part_phrase_boundary_hypothesis> result;
    result.push_back(diversion_part(11, {
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            phrase_boundary_evidence_polarity::supports,
            0.86,
            "arrival-like timing"),
        evidence(
            phrase_boundary_evidence_kind::cross_boundary_continuity,
            phrase_boundary_evidence_origin::motif_analysis,
            phrase_boundary_evidence_polarity::counters,
            0.91,
            "part 11 continues"),
    }));
    result.push_back(diversion_part(12, {
        evidence(
            phrase_boundary_evidence_kind::motif_completion,
            phrase_boundary_evidence_origin::motif_analysis,
            phrase_boundary_evidence_polarity::supports,
            0.84,
            "arrival-like motif cue"),
        evidence(
            phrase_boundary_evidence_kind::cross_boundary_continuity,
            phrase_boundary_evidence_origin::motif_analysis,
            phrase_boundary_evidence_polarity::counters,
            0.88,
            "part 12 continues"),
    }));
    return result;
}

part_phrase_boundary_hypothesis closing_part(node_id part_id, std::int64_t tick) {
    part_phrase_boundary_hypothesis result;
    result.part_id = part_id;
    result.boundary = make_phrase_boundary_hypothesis(
        at(tick),
        0.92,
        {
            evidence(
                phrase_boundary_evidence_kind::motif_completion,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::supports,
                0.90,
                "final motif completion"),
            evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                phrase_boundary_evidence_polarity::supports,
                0.88,
                "final timing support"),
        });
    return result;
}

phrase_boundary_consensus closing_boundary(std::int64_t tick) {
    std::vector<part_phrase_boundary_hypothesis> parts;
    parts.push_back(closing_part(101, tick));
    parts.push_back(closing_part(102, tick));
    return make_phrase_boundary_consensus(std::move(parts));
}

cadential_arrival_hypothesis closing_arrival(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second,
    const phrase_boundary_consensus& boundary) {
    const auto transition = infer_harmonic_transition(first, second);
    return infer_cadential_arrival(
        boundary,
        transition,
        voices(
            first.projection.source_verticality.observation_time.tick,
            second.projection.source_verticality.observation_time.tick));
}
} // namespace

int main() {
    const auto key = c_ionian_key();
    const auto g_diversion = triad(7, tertian_triad_quality::major, 100);
    const auto a_minor = triad(9, tertian_triad_quality::minor, 200);
    const auto g_later = triad(7, tertian_triad_quality::major, 300);
    const auto c_later = triad(0, tertian_triad_quality::major, 400);
    const auto final_boundary = closing_boundary(400);
    const auto final_arrival = closing_arrival(g_later, c_later, final_boundary);
    const auto final_voices = voices(300, 400);
    const auto conflict_parts = strong_deviation_conflict();

    const auto conflict = infer_cadential_deviation_conflict_evidence(
        conflict_parts,
        at(200));
    CHECK(conflict.arrival_like_punctuation_grounded);
    CHECK(conflict.continuation_cross_part_grounded);
    CHECK(conflict.deviation_conflict_grounded);
    CHECK(conflict.punctuation_supporting_parts.size() == 2);
    CHECK(conflict.continuation_supporting_parts.size() == 2);
    CHECK(conflict.punctuation_support_domains == 2);
    CHECK(close_enough(conflict.confidence, cadential_deviation_conflict_ceiling));

    // Strong common-practice candidate: an arrival-like V -> VI moment is
    // contradicted by cross-part continuation, then an authentic closure occurs
    // later. The label is still explicitly a candidate, not established truth.
    const auto deceptive = infer_ionian_deceptive_cadence_candidate(
        key,
        g_diversion,
        a_minor,
        conflict_parts,
        g_later,
        c_later,
        final_arrival,
        final_boundary,
        final_voices);
    CHECK(deceptive.five_to_six_diversion_grounded);
    CHECK(deceptive.deviation_conflict_grounded);
    CHECK(deceptive.deferred_authentic_resolution_grounded);
    CHECK(deceptive.deceptive_cadence_candidate_resolved);
    CHECK(deceptive.kind ==
        ionian_deceptive_cadence_candidate_kind::deceptive_cadence_candidate);
    CHECK(close_enough(
        deceptive.confidence,
        ionian_deceptive_cadence_candidate_ceiling));
    CHECK(!deceptive.deceptive_cadence_established);
    CHECK(!deceptive.roman_numeral_named);

    // Continuation plus later authentic closure is evidence of deferral, but
    // without an arrival-like punctuation conflict it is not yet a deceptive
    // cadence candidate.
    const std::vector<part_phrase_boundary_hypothesis> continuation_only{
        diversion_part(11, {
            evidence(
                phrase_boundary_evidence_kind::cross_boundary_continuity,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::counters,
                0.91,
                "part 11 continues"),
        }),
        diversion_part(12, {
            evidence(
                phrase_boundary_evidence_kind::cross_boundary_continuity,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::counters,
                0.88,
                "part 12 continues"),
        }),
    };
    const auto no_punctuation = infer_ionian_deceptive_cadence_candidate(
        key,
        g_diversion,
        a_minor,
        continuation_only,
        g_later,
        c_later,
        final_arrival,
        final_boundary,
        final_voices);
    CHECK(no_punctuation.deferred_authentic_resolution_grounded);
    CHECK(!no_punctuation.deviation_conflict_grounded);
    CHECK(!no_punctuation.deceptive_cadence_candidate_resolved);

    // A cadence detector cannot circularly supply the punctuation evidence used
    // to justify the deceptive cadence label.
    const std::vector<part_phrase_boundary_hypothesis> circular_punctuation{
        diversion_part(11, {
            evidence(
                phrase_boundary_evidence_kind::cadence_or_resolution,
                phrase_boundary_evidence_origin::harmonic_analysis,
                phrase_boundary_evidence_polarity::supports,
                0.96,
                "upstream cadence detector"),
            evidence(
                phrase_boundary_evidence_kind::cross_boundary_continuity,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::counters,
                0.91,
                "part 11 continues"),
        }),
        diversion_part(12, {
            evidence(
                phrase_boundary_evidence_kind::cadence_or_resolution,
                phrase_boundary_evidence_origin::harmonic_analysis,
                phrase_boundary_evidence_polarity::supports,
                0.95,
                "upstream cadence detector"),
            evidence(
                phrase_boundary_evidence_kind::cross_boundary_continuity,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::counters,
                0.89,
                "part 12 continues"),
        }),
    };
    const auto circular = infer_cadential_deviation_conflict_evidence(
        circular_punctuation,
        at(200));
    CHECK(circular.continuation_cross_part_grounded);
    CHECK(!circular.arrival_like_punctuation_grounded);
    CHECK(!circular.deviation_conflict_grounded);

    // Two parts with the same timing cue are still only one evidence domain.
    // The candidate requires independent punctuation domains rather than a
    // duplicated detector output.
    const std::vector<part_phrase_boundary_hypothesis> one_domain{
        diversion_part(11, {
            evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                phrase_boundary_evidence_polarity::supports,
                0.88,
                "timing cue A"),
            evidence(
                phrase_boundary_evidence_kind::cross_boundary_continuity,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::counters,
                0.91,
                "part 11 continues"),
        }),
        diversion_part(12, {
            evidence(
                phrase_boundary_evidence_kind::part_density_change,
                phrase_boundary_evidence_origin::performance_timing,
                phrase_boundary_evidence_polarity::supports,
                0.86,
                "timing-domain density cue"),
            evidence(
                phrase_boundary_evidence_kind::cross_boundary_continuity,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::counters,
                0.89,
                "part 12 continues"),
        }),
    };
    const auto duplicated_domain = infer_cadential_deviation_conflict_evidence(
        one_domain,
        at(200));
    CHECK(duplicated_domain.punctuation_supporting_parts.size() == 2);
    CHECK(duplicated_domain.punctuation_support_domains == 1);
    CHECK(!duplicated_domain.arrival_like_punctuation_grounded);
    CHECK(!duplicated_domain.deviation_conflict_grounded);

    // If the later event does not become an authentic closure, the same local
    // deviation conflict remains interesting but cannot mature into this
    // specific deceptive-cadence candidate.
    const auto a_later = triad(9, tertian_triad_quality::minor, 400);
    const auto nonauth_boundary = closing_boundary(400);
    const auto nonauth_arrival = closing_arrival(g_later, a_later, nonauth_boundary);
    const auto no_deferred_authentic = infer_ionian_deceptive_cadence_candidate(
        key,
        g_diversion,
        a_minor,
        conflict_parts,
        g_later,
        a_later,
        nonauth_arrival,
        nonauth_boundary,
        final_voices);
    CHECK(no_deferred_authentic.deviation_conflict_grounded);
    CHECK(!no_deferred_authentic.deferred_authentic_resolution_grounded);
    CHECK(!no_deferred_authentic.deceptive_cadence_candidate_resolved);

    // An actual V -> I at the earlier moment is not a deceptive diversion even
    // if its surface grouping contains conflicting continuation cues.
    const auto c_at_diversion = triad(0, tertian_triad_quality::major, 200);
    const auto not_five_to_six = infer_ionian_deceptive_cadence_candidate(
        key,
        g_diversion,
        c_at_diversion,
        conflict_parts,
        g_later,
        c_later,
        final_arrival,
        final_boundary,
        final_voices);
    CHECK(!not_five_to_six.five_to_six_diversion_grounded);
    CHECK(!not_five_to_six.deceptive_cadence_candidate_resolved);

    return 0;
}
