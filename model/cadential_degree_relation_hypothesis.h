#pragma once

#include "cadential_arrival_hypothesis.h"
#include "diatonic_chord_degree_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace vgmtooling::model {

// This layer names only key-relative scale-degree motion at an independently
// grounded phrase/harmonic arrival. It deliberately stops below Roman numeral,
// tonal-function, and cadence-class interpretation. A 5 -> 1 arrival is useful
// structural evidence, but PAC/IAC still depend on voicing, inversion, melodic
// closure, and style/context that are not encoded by scale degree alone.
enum class cadential_degree_relation_kind : std::uint8_t {
    unresolved = 0,
    five_to_one_arrival,
    five_to_six_arrival,
    arrival_on_five,
    other_diatonic_arrival,
};

struct cadential_degree_relation_hypothesis {
    time_coordinate first_time{};
    time_coordinate arrival_time{};
    std::int64_t key_center_pitch_class = 0;
    diatonic_mode key_mode = diatonic_mode::ionian;
    std::optional<std::uint8_t> first_scale_degree{};
    std::optional<std::uint8_t> arrival_scale_degree{};
    std::int64_t first_root_pitch_class = 0;
    std::int64_t arrival_root_pitch_class = 0;
    std::int64_t root_motion_semitones = 0;
    std::int64_t root_interval_class = 0;
    cadential_degree_relation_kind kind = cadential_degree_relation_kind::unresolved;
    bool phrase_grounded = false;
    bool root_motion_grounded = false;
    bool voice_leading_grounded = false;
    bool roman_numeral_named = false;
    bool tonal_function_named = false;
    bool cadence_class_named = false;
    double confidence = 0.0;
    std::string theory_scope =
        "key-relative diatonic degree motion at phrase-aligned harmonic arrival";
};

constexpr double unresolved_cadential_degree_relation_ceiling = 0.55;
constexpr double cadential_degree_relation_confidence_ceiling = 0.82;

inline const char* to_string(cadential_degree_relation_kind kind) noexcept {
    switch (kind) {
    case cadential_degree_relation_kind::unresolved:
        return "unresolved";
    case cadential_degree_relation_kind::five_to_one_arrival:
        return "five_to_one_arrival";
    case cadential_degree_relation_kind::five_to_six_arrival:
        return "five_to_six_arrival";
    case cadential_degree_relation_kind::arrival_on_five:
        return "arrival_on_five";
    case cadential_degree_relation_kind::other_diatonic_arrival:
        return "other_diatonic_arrival";
    }
    return "unknown";
}

inline bool cadential_degree_same_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
        first.tick_rate == second.tick_rate &&
        first.loop_iteration == second.loop_iteration;
}

inline void validate_cadential_degree_input(
    const diatonic_chord_degree_hypothesis& degree) {
    if (!std::isfinite(degree.confidence) ||
        degree.confidence < 0.0 || degree.confidence > 1.0) {
        throw std::invalid_argument(
            "cadential degree input confidence must be finite in [0, 1]");
    }
    if (degree.scale_degree.has_value() &&
        (*degree.scale_degree < 1 || *degree.scale_degree > 7)) {
        throw std::invalid_argument(
            "cadential degree input scale degree must lie in [1, 7]");
    }
}

inline cadential_degree_relation_hypothesis infer_cadential_degree_relation(
    const tonal_key_class_hypothesis& key,
    const tertian_triad_hypothesis& first_chord,
    const tertian_triad_hypothesis& arrival_chord,
    const cadential_arrival_hypothesis& arrival) {
    // Re-derive both degrees from the actual key + chord hypotheses here rather
    // than accepting detached degree summaries. This preserves the key region,
    // tuning, pitch-role, root-ambiguity, and confidence contracts enforced by
    // the lower layer and prevents provenance-compatible-looking labels from
    // being loaned across unrelated observations.
    const auto first = infer_diatonic_chord_degree_hypothesis(key, first_chord);
    const auto arrival_degree = infer_diatonic_chord_degree_hypothesis(key, arrival_chord);
    validate_cadential_degree_input(first);
    validate_cadential_degree_input(arrival_degree);
    if (!std::isfinite(arrival.confidence) ||
        arrival.confidence < 0.0 || arrival.confidence > 1.0) {
        throw std::invalid_argument(
            "cadential degree arrival confidence must be finite in [0, 1]");
    }

    if (!cadential_degree_same_time_basis(first.observation_time, arrival_degree.observation_time) ||
        arrival_degree.observation_time.tick <= first.observation_time.tick) {
        throw std::invalid_argument(
            "cadential degree relation requires ordered chord degrees in one time basis");
    }
    if (!cadential_degree_same_time_basis(first.observation_time, arrival.departure_time) ||
        first.observation_time.tick != arrival.departure_time.tick ||
        !cadential_degree_same_time_basis(arrival_degree.observation_time, arrival.arrival_time) ||
        arrival_degree.observation_time.tick != arrival.arrival_time.tick) {
        throw std::invalid_argument(
            "cadential degree relation chords must be the exact harmonic transition that licensed the arrival");
    }

    const std::int64_t expected_motion = positive_mod(
        arrival_degree.chord_root_pitch_class - first.chord_root_pitch_class,
        12);
    const std::int64_t expected_interval_class = std::min(
        expected_motion,
        12 - expected_motion);
    if (arrival.root_motion_semitones != expected_motion ||
        arrival.root_interval_class != expected_interval_class) {
        throw std::invalid_argument(
            "cadential degree roots disagree with the harmonic-arrival motion");
    }

    cadential_degree_relation_hypothesis result;
    result.first_time = first.observation_time;
    result.arrival_time = arrival_degree.observation_time;
    result.key_center_pitch_class = first.key_center_pitch_class;
    result.key_mode = first.key_mode;
    result.first_scale_degree = first.scale_degree;
    result.arrival_scale_degree = arrival_degree.scale_degree;
    result.first_root_pitch_class = first.chord_root_pitch_class;
    result.arrival_root_pitch_class = arrival_degree.chord_root_pitch_class;
    result.root_motion_semitones = expected_motion;
    result.root_interval_class = expected_interval_class;
    result.phrase_grounded = arrival.cross_part_phrase_grounded;
    result.root_motion_grounded = arrival.harmonic_root_motion_reliable;
    result.voice_leading_grounded = arrival.voice_leading_grounded;
    result.confidence = std::min({
        first.confidence,
        arrival_degree.confidence,
        arrival.confidence,
        cadential_degree_relation_confidence_ceiling,
    });

    // A chromatic or otherwise unresolved root remains useful negative evidence,
    // but it cannot be forced into a diatonic cadence-shaped relation.
    if (!first.scale_degree.has_value() || !arrival_degree.scale_degree.has_value() ||
        first.chromatic_root || arrival_degree.chromatic_root ||
        !result.phrase_grounded || !result.root_motion_grounded) {
        result.kind = cadential_degree_relation_kind::unresolved;
        result.confidence = std::min(
            result.confidence,
            unresolved_cadential_degree_relation_ceiling);
        return result;
    }

    const auto first_degree = *first.scale_degree;
    const auto second_degree = *arrival_degree.scale_degree;
    if (first_degree == 5 && second_degree == 1) {
        result.kind = cadential_degree_relation_kind::five_to_one_arrival;
    } else if (first_degree == 5 && second_degree == 6) {
        result.kind = cadential_degree_relation_kind::five_to_six_arrival;
    } else if (second_degree == 5) {
        result.kind = cadential_degree_relation_kind::arrival_on_five;
    } else {
        result.kind = cadential_degree_relation_kind::other_diatonic_arrival;
    }

    // These remain explicitly false. Scale-degree motion at a phrase boundary
    // is a dependency for richer interpretation, not permission to invent it.
    result.roman_numeral_named = false;
    result.tonal_function_named = false;
    result.cadence_class_named = false;
    return result;
}

} // namespace vgmtooling::model
