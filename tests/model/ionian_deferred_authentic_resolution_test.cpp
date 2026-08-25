#include "model/ionian_deferred_authentic_resolution.h"

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

phrase_boundary_evidence boundary_evidence(
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
    result.detail = "deferred authentic resolution regression";
    return result;
}

part_phrase_boundary_hypothesis continuation_part(
    node_id part_id,
    std::int64_t tick,
    double confidence = 0.90) {
    part_phrase_boundary_hypothesis result;
    result.part_id = part_id;
    result.boundary = make_phrase_boundary_hypothesis(
        at(tick),
        0.90,
        {boundary_evidence(
            phrase_boundary_evidence_kind::cross_boundary_continuity,
            phrase_boundary_evidence_origin::motif_analysis,
            phrase_boundary_evidence_polarity::counters,
            confidence,
            "cross-boundary continuation")});
    return result;
}

part_phrase_boundary_hypothesis closing_part(node_id part_id, std::int64_t tick) {
    part_phrase_boundary_hypothesis result;
    result.part_id = part_id;
    result.boundary = make_phrase_boundary_hypothesis(
        at(tick),
        0.92,
        {
            boundary_evidence(
                phrase_boundary_evidence_kind::motif_completion,
                phrase_boundary_evidence_origin::motif_analysis,
                phrase_boundary_evidence_polarity::supports,
                0.90,
                "motif completion"),
            boundary_evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                phrase_boundary_evidence_polarity::supports,
                0.88,
                "timing support"),
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
    const auto first_tick = first.projection.source_verticality.observation_time.tick;
    const auto second_tick = second.projection.source_verticality.observation_time.tick;
    return infer_cadential_arrival(
        boundary,
        transition,
        voices(first_tick, second_tick));
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

    std::vector<part_phrase_boundary_hypothesis> continuation;
    continuation.push_back(continuation_part(11, 200, 0.91));
    continuation.push_back(continuation_part(12, 200, 0.87));

    // Strong case: V -> VI is followed by cross-part continuation, then a later
    // independently grounded V -> I phrase closure. This supports deferred
    // authentic resolution, but still does not name a deceptive cadence.
    const auto deferred = infer_ionian_deferred_authentic_resolution(
        key,
        g_diversion,
        a_minor,
        continuation,
        g_later,
        c_later,
        final_arrival,
        final_boundary,
        final_voices);
    CHECK(deferred.deferred_resolution_candidate_resolved);
    CHECK(deferred.kind ==
        ionian_deferred_resolution_kind::deferred_authentic_resolution_candidate);
    CHECK(deferred.diversion_source_degree.has_value());
    CHECK(*deferred.diversion_source_degree == 5);
    CHECK(deferred.diversion_target_degree.has_value());
    CHECK(*deferred.diversion_target_degree == 6);
    CHECK(deferred.diversion_diatonic);
    CHECK(deferred.diversion_root_motion_reliable);
    CHECK(deferred.continuation_cross_part_grounded);
    CHECK(deferred.later_authentic_candidate_grounded);
    CHECK(authentic_morphology_kind(deferred.later_authentic_kind));
    CHECK(close_enough(deferred.confidence, deferred_authentic_resolution_ceiling));
    CHECK(!deferred.deceptive_cadence_named);
    CHECK(!deferred.cadence_class_established);

    // V -> VI with only one continuing part is not enough. One line carrying on
    // may be local figuration rather than evidence that the phrase as a whole
    // deliberately deferred closure.
    const std::vector<part_phrase_boundary_hypothesis> one_part{
        continuation_part(11, 200, 0.95),
    };
    const auto local_only = infer_ionian_deferred_authentic_resolution(
        key,
        g_diversion,
        a_minor,
        one_part,
        g_later,
        c_later,
        final_arrival,
        final_boundary,
        final_voices);
    CHECK(!local_only.continuation_cross_part_grounded);
    CHECK(!local_only.deferred_resolution_candidate_resolved);
    CHECK(local_only.kind == ionian_deferred_resolution_kind::unresolved);

    // Weak continuation claims below the use threshold do not mature merely
    // because two part IDs are present.
    const std::vector<part_phrase_boundary_hypothesis> weak_continuation{
        continuation_part(11, 200, 0.69),
        continuation_part(12, 200, 0.69),
    };
    const auto weak = infer_ionian_deferred_authentic_resolution(
        key,
        g_diversion,
        a_minor,
        weak_continuation,
        g_later,
        c_later,
        final_arrival,
        final_boundary,
        final_voices);
    CHECK(!weak.continuation_cross_part_grounded);
    CHECK(!weak.deferred_resolution_candidate_resolved);

    // Even strong V -> VI continuation is only a continuation candidate if the
    // later event does not actually become an integrated authentic closure.
    const auto a_later = triad(9, tertian_triad_quality::minor, 400);
    const auto nonauth_boundary = closing_boundary(400);
    const auto nonauth_arrival = closing_arrival(g_later, a_later, nonauth_boundary);
    const auto no_later_authentic = infer_ionian_deferred_authentic_resolution(
        key,
        g_diversion,
        a_minor,
        continuation,
        g_later,
        a_later,
        nonauth_arrival,
        nonauth_boundary,
        final_voices);
    CHECK(no_later_authentic.kind ==
        ionian_deferred_resolution_kind::five_to_six_continuation_candidate);
    CHECK(!no_later_authentic.later_authentic_candidate_grounded);
    CHECK(!no_later_authentic.deferred_resolution_candidate_resolved);
    CHECK(!no_later_authentic.deceptive_cadence_named);

    // An ordinary V -> I event followed by continuation is not a deceptive
    // diversion merely because a later authentic cadence also occurs.
    const auto c_diversion_target = triad(0, tertian_triad_quality::major, 200);
    const auto not_diverted = infer_ionian_deferred_authentic_resolution(
        key,
        g_diversion,
        c_diversion_target,
        continuation,
        g_later,
        c_later,
        final_arrival,
        final_boundary,
        final_voices);
    CHECK(!not_diverted.deferred_resolution_candidate_resolved);
    CHECK(not_diverted.kind == ionian_deferred_resolution_kind::unresolved);

    // Two reports from the same persistent part are still one source of musical
    // continuity, not cross-part corroboration.
    const std::vector<part_phrase_boundary_hypothesis> duplicate_part{
        continuation_part(11, 200, 0.95),
        continuation_part(11, 200, 0.92),
    };
    const auto duplicate = infer_cross_part_continuation_evidence(
        duplicate_part,
        at(200));
    CHECK(!duplicate.cross_part_grounded);
    CHECK(duplicate.supporting_parts.size() == 1);

    bool stale_continuation_rejected = false;
    try {
        auto stale = continuation;
        stale[1].boundary.boundary.tick = 201;
        (void)infer_ionian_deferred_authentic_resolution(
            key,
            g_diversion,
            a_minor,
            stale,
            g_later,
            c_later,
            final_arrival,
            final_boundary,
            final_voices);
    } catch (const std::invalid_argument&) {
        stale_continuation_rejected = true;
    }
    CHECK(stale_continuation_rejected);

    // A so-called later authentic cadence whose dominant begins before the
    // diversion is chronologically incompatible and must be rejected.
    bool overlapping_later_cadence_rejected = false;
    try {
        const auto early_g = triad(7, tertian_triad_quality::major, 150);
        const auto later_c = triad(0, tertian_triad_quality::major, 400);
        const auto boundary400 = closing_boundary(400);
        const auto arrival150_400 = closing_arrival(early_g, later_c, boundary400);
        const auto voices150_400 = voices(150, 400);
        (void)infer_ionian_deferred_authentic_resolution(
            key,
            g_diversion,
            a_minor,
            continuation,
            early_g,
            later_c,
            arrival150_400,
            boundary400,
            voices150_400);
    } catch (const std::invalid_argument&) {
        overlapping_later_cadence_rejected = true;
    }
    CHECK(overlapping_later_cadence_rejected);

    return 0;
}
