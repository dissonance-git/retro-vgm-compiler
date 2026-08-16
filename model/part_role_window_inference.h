#pragma once

#include "musical_part_role_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// A bounded signal keeps "how much of this property is present" separate from
// "how certain are we that the measurement is valid". Role evidence uses their
// product so a large but poorly grounded measurement cannot become strong by
// accident.
struct bounded_role_signal {
    double value = 0.0;
    double confidence = 0.0;
};

struct part_role_window_descriptor {
    node_id part_id = 0;
    time_span active{};
    std::size_t onset_count = 0;

    // Register is representation-local. It can help establish relative position
    // only when every compared part uses the same basis.
    std::optional<double> register_coordinate{};
    std::string register_basis;

    // These are outputs of other analyses, not labels supplied by a composer
    // classifier. Missing means unknown, not zero.
    std::optional<bounded_role_signal> auditory_salience{};
    std::optional<bounded_role_signal> structural_motif_prominence{};
    std::optional<bounded_role_signal> phrase_boundary_participation{};
    std::optional<bounded_role_signal> harmonic_bass_ownership{};
    std::optional<bounded_role_signal> counterpoint_independence{};
    std::optional<bounded_role_signal> imitation_or_response{};
    std::optional<bounded_role_signal> rhythmic_repetition{};
    std::optional<bounded_role_signal> sustained_texture{};
    std::optional<bounded_role_signal> percussion_identity{};
    std::optional<bounded_role_signal> doubling_correspondence{};
};

struct inferred_part_role_candidate {
    node_id part_id = 0;
    musical_part_role role = musical_part_role::unresolved;
    musical_part_role_hypothesis hypothesis{};
};

struct part_role_window_result {
    time_span active{};
    std::vector<inferred_part_role_candidate> candidates;
};

constexpr double role_signal_use_threshold = 0.55;
constexpr double role_signal_strong_threshold = 0.75;
constexpr double auto_role_register_evidence_ceiling = 0.72;
constexpr double auto_role_activity_evidence_ceiling = 0.70;

inline void validate_bounded_role_signal(const bounded_role_signal& signal) {
    if (!std::isfinite(signal.value) || signal.value < 0.0 || signal.value > 1.0 ||
        !std::isfinite(signal.confidence) || signal.confidence < 0.0 || signal.confidence > 1.0) {
        throw std::invalid_argument("bounded role signal value/confidence must be finite in [0, 1]");
    }
}

inline double bounded_role_signal_strength(const bounded_role_signal& signal) {
    validate_bounded_role_signal(signal);
    return signal.value * signal.confidence;
}

inline bool same_role_window_span(const time_span& first, const time_span& second) noexcept {
    return first.end.has_value() && second.end.has_value() &&
        first.start == second.start && *first.end == *second.end;
}

inline void validate_part_role_window_descriptor(const part_role_window_descriptor& descriptor) {
    if (descriptor.part_id == 0)
        throw std::invalid_argument("role-window descriptor requires a nonzero persistent-part id");
    if (!descriptor.active.end.has_value() ||
        !part_role_same_time_basis(descriptor.active.start, *descriptor.active.end) ||
        descriptor.active.end->tick <= descriptor.active.start.tick) {
        throw std::invalid_argument("role-window descriptor requires a positive bounded span");
    }
    if (descriptor.register_coordinate.has_value()) {
        if (!std::isfinite(*descriptor.register_coordinate) || descriptor.register_basis.empty())
            throw std::invalid_argument("role-window register coordinate requires a finite value and basis");
    } else if (!descriptor.register_basis.empty()) {
        throw std::invalid_argument("role-window register basis requires a coordinate");
    }

    for (const auto* signal : {
             descriptor.auditory_salience ? &*descriptor.auditory_salience : nullptr,
             descriptor.structural_motif_prominence ? &*descriptor.structural_motif_prominence : nullptr,
             descriptor.phrase_boundary_participation ? &*descriptor.phrase_boundary_participation : nullptr,
             descriptor.harmonic_bass_ownership ? &*descriptor.harmonic_bass_ownership : nullptr,
             descriptor.counterpoint_independence ? &*descriptor.counterpoint_independence : nullptr,
             descriptor.imitation_or_response ? &*descriptor.imitation_or_response : nullptr,
             descriptor.rhythmic_repetition ? &*descriptor.rhythmic_repetition : nullptr,
             descriptor.sustained_texture ? &*descriptor.sustained_texture : nullptr,
             descriptor.percussion_identity ? &*descriptor.percussion_identity : nullptr,
             descriptor.doubling_correspondence ? &*descriptor.doubling_correspondence : nullptr,
         }) {
        if (signal != nullptr)
            validate_bounded_role_signal(*signal);
    }
}

inline std::optional<double> role_signal_strength_if_usable(
    const std::optional<bounded_role_signal>& signal) {
    if (!signal.has_value())
        return std::nullopt;
    const double strength = bounded_role_signal_strength(*signal);
    if (strength < role_signal_use_threshold)
        return std::nullopt;
    return strength;
}

inline part_role_evidence automatic_role_signal_evidence(
    part_role_evidence_kind kind,
    double confidence,
    std::string detail,
    std::string source,
    node_id part_id,
    part_role_evidence_origin origin = part_role_evidence_origin::musical_analysis) {
    return {
        kind,
        origin,
        part_role_evidence_polarity::supports,
        evidence_status::hypothesis,
        confidence,
        std::move(source),
        std::move(detail),
        {part_id},
    };
}

inline double role_evidence_mean(const std::vector<part_role_evidence>& evidence) {
    if (evidence.empty())
        return 0.0;
    double total = 0.0;
    std::size_t support_count = 0;
    for (const auto& item : evidence) {
        if (item.polarity != part_role_evidence_polarity::supports)
            continue;
        total += item.confidence;
        ++support_count;
    }
    return support_count == 0 ? 0.0 : total / static_cast<double>(support_count);
}

inline void append_auto_role_candidate(
    part_role_window_result& result,
    const part_role_window_descriptor& descriptor,
    musical_part_role role,
    std::vector<part_role_evidence> evidence) {
    if (evidence.empty())
        return;
    const double proposed = role_evidence_mean(evidence);
    if (proposed <= 0.0)
        return;
    auto hypothesis = make_musical_part_role_hypothesis(
        descriptor.part_id,
        role,
        descriptor.active,
        proposed,
        std::move(evidence));
    result.candidates.push_back({descriptor.part_id, role, std::move(hypothesis)});
}

inline part_role_window_result infer_part_roles_for_window(
    const std::vector<part_role_window_descriptor>& descriptors,
    std::string source) {
    if (descriptors.empty())
        throw std::invalid_argument("automatic role inference requires at least one persistent part");
    if (source.empty())
        throw std::invalid_argument("automatic role inference requires a source");

    for (const auto& descriptor : descriptors)
        validate_part_role_window_descriptor(descriptor);
    for (std::size_t index = 1; index < descriptors.size(); ++index) {
        if (!same_role_window_span(descriptors.front().active, descriptors[index].active))
            throw std::invalid_argument("automatic role inference requires one synchronized analysis window");
    }

    part_role_window_result result;
    result.active = descriptors.front().active;

    std::size_t max_onsets = 0;
    for (const auto& descriptor : descriptors)
        max_onsets = std::max(max_onsets, descriptor.onset_count);

    bool common_register_basis = descriptors.size() >= 2;
    std::string register_basis;
    for (const auto& descriptor : descriptors) {
        if (!descriptor.register_coordinate.has_value()) {
            common_register_basis = false;
            break;
        }
        if (register_basis.empty())
            register_basis = descriptor.register_basis;
        else if (descriptor.register_basis != register_basis) {
            common_register_basis = false;
            break;
        }
    }

    std::optional<node_id> unique_lowest_part;
    std::optional<node_id> unique_highest_part;
    if (common_register_basis) {
        double low = *descriptors.front().register_coordinate;
        double high = low;
        node_id low_part = descriptors.front().part_id;
        node_id high_part = descriptors.front().part_id;
        bool low_tie = false;
        bool high_tie = false;
        for (std::size_t index = 1; index < descriptors.size(); ++index) {
            const double value = *descriptors[index].register_coordinate;
            if (value < low - 1e-9) {
                low = value;
                low_part = descriptors[index].part_id;
                low_tie = false;
            } else if (std::fabs(value - low) <= 1e-9) {
                low_tie = true;
            }
            if (value > high + 1e-9) {
                high = value;
                high_part = descriptors[index].part_id;
                high_tie = false;
            } else if (std::fabs(value - high) <= 1e-9) {
                high_tie = true;
            }
        }
        if (!low_tie)
            unique_lowest_part = low_part;
        if (!high_tie)
            unique_highest_part = high_part;
    }

    double strongest_foreground_signal = 0.0;
    node_id strongest_foreground_part = 0;
    for (const auto& descriptor : descriptors) {
        double signal = 0.0;
        if (const auto motif = role_signal_strength_if_usable(descriptor.structural_motif_prominence))
            signal += *motif;
        if (const auto phrase = role_signal_strength_if_usable(descriptor.phrase_boundary_participation))
            signal += *phrase;
        if (const auto salience = role_signal_strength_if_usable(descriptor.auditory_salience))
            signal += *salience;
        if (signal > strongest_foreground_signal) {
            strongest_foreground_signal = signal;
            strongest_foreground_part = descriptor.part_id;
        }
    }

    for (const auto& descriptor : descriptors) {
        const double activity = max_onsets == 0
            ? 0.0
            : static_cast<double>(descriptor.onset_count) / static_cast<double>(max_onsets);

        // Bass foundation: harmonic ownership is the meaningful relation. Being
        // the lowest register is corroboration only, never sufficient by itself.
        {
            std::vector<part_role_evidence> evidence;
            if (const auto bass = role_signal_strength_if_usable(descriptor.harmonic_bass_ownership)) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::harmonic_bass_ownership,
                    *bass,
                    "independent harmonic analysis assigns persistent low-voice ownership to this part",
                    source,
                    descriptor.part_id));
            }
            if (unique_lowest_part.has_value() && *unique_lowest_part == descriptor.part_id) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::register_position,
                    auto_role_register_evidence_ceiling,
                    "part is uniquely lowest within a common representation-local register basis",
                    source,
                    descriptor.part_id,
                    part_role_evidence_origin::synthesis_runtime));
            }
            if (role_signal_strength_if_usable(descriptor.harmonic_bass_ownership).has_value())
                append_auto_role_candidate(result, descriptor, musical_part_role::bass_foundation, std::move(evidence));
        }

        // Foreground melody requires structural material evidence. Register and
        // activity can corroborate but cannot create the role.
        {
            std::vector<part_role_evidence> evidence;
            const auto motif = role_signal_strength_if_usable(descriptor.structural_motif_prominence);
            const auto phrase = role_signal_strength_if_usable(descriptor.phrase_boundary_participation);
            const auto salience = role_signal_strength_if_usable(descriptor.auditory_salience);
            if (motif.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::melodic_motif_prominence,
                    *motif,
                    "part carries structurally prominent motivic material in this window",
                    source,
                    descriptor.part_id));
            }
            if (phrase.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::phrase_initiation_or_completion,
                    *phrase,
                    "part participates strongly in phrase initiation/completion",
                    source,
                    descriptor.part_id));
            }
            if (salience.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::auditory_salience,
                    *salience,
                    "auditory analysis independently places the part toward the foreground",
                    source,
                    descriptor.part_id,
                    part_role_evidence_origin::auditory_analysis));
            }
            if (unique_highest_part.has_value() && *unique_highest_part == descriptor.part_id) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::register_position,
                    std::min(auto_role_register_evidence_ceiling, 0.62),
                    "part is uniquely highest within the common register basis; corroborative only",
                    source,
                    descriptor.part_id,
                    part_role_evidence_origin::synthesis_runtime));
            }
            if (motif.has_value() && (phrase.has_value() || salience.has_value()))
                append_auto_role_candidate(result, descriptor, musical_part_role::melodic_foreground, std::move(evidence));
        }

        // Counterline: independent motion plus response/imitation is a stronger
        // combination than either alone. Counterpoint alone remains capped by
        // the role kernel's single-domain rule.
        {
            std::vector<part_role_evidence> evidence;
            const auto counterpoint = role_signal_strength_if_usable(descriptor.counterpoint_independence);
            const auto response = role_signal_strength_if_usable(descriptor.imitation_or_response);
            const auto salience = role_signal_strength_if_usable(descriptor.auditory_salience);
            if (counterpoint.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::counterpoint_independence,
                    std::min(*counterpoint, 0.72),
                    "part moves independently against another persistent line",
                    source,
                    descriptor.part_id));
            }
            if (response.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::imitation_or_response,
                    *response,
                    "part answers or imitates material from another persistent line",
                    source,
                    descriptor.part_id));
            }
            if (salience.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::auditory_salience,
                    *salience,
                    "auditory analysis supports an independently audible secondary line",
                    source,
                    descriptor.part_id,
                    part_role_evidence_origin::auditory_analysis));
            }
            if (counterpoint.has_value() && response.has_value())
                append_auto_role_candidate(result, descriptor, musical_part_role::counterline, std::move(evidence));
        }

        // Ostinato: recurring rhythmic material plus sustained activity is a
        // relational role. Activity alone remains realization evidence.
        {
            std::vector<part_role_evidence> evidence;
            const auto repetition = role_signal_strength_if_usable(descriptor.rhythmic_repetition);
            if (repetition.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::rhythmic_ostinato,
                    *repetition,
                    "part repeats a stable rhythmic cell within the synchronized window",
                    source,
                    descriptor.part_id));
            }
            if (activity >= role_signal_use_threshold && max_onsets > 0) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::activity_density,
                    std::min(activity, auto_role_activity_evidence_ceiling),
                    "part remains relatively active across the window",
                    source,
                    descriptor.part_id,
                    part_role_evidence_origin::synthesis_runtime));
            }
            if (repetition.has_value() && activity >= role_signal_use_threshold)
                append_auto_role_candidate(result, descriptor, musical_part_role::ostinato, std::move(evidence));
        }

        // Sustained support does not follow from low activity by itself. It
        // requires a positive sustained-texture analysis, optionally reinforced
        // by lower auditory salience than a stronger foreground candidate.
        {
            std::vector<part_role_evidence> evidence;
            const auto sustained = role_signal_strength_if_usable(descriptor.sustained_texture);
            if (sustained.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::sustained_texture,
                    *sustained,
                    "part supplies sustained textural support in this window",
                    source,
                    descriptor.part_id));
            }
            if (descriptor.auditory_salience.has_value() &&
                strongest_foreground_part != 0 && strongest_foreground_part != descriptor.part_id) {
                const auto& salience = *descriptor.auditory_salience;
                const double low_salience_support = (1.0 - salience.value) * salience.confidence;
                if (low_salience_support >= role_signal_use_threshold) {
                    evidence.push_back(automatic_role_signal_evidence(
                        part_role_evidence_kind::auditory_salience,
                        low_salience_support,
                        "part is independently less salient than the strongest foreground candidate",
                        source,
                        descriptor.part_id,
                        part_role_evidence_origin::auditory_analysis));
                }
            }
            if (sustained.has_value())
                append_auto_role_candidate(result, descriptor, musical_part_role::sustained_support, std::move(evidence));
        }

        // Accompaniment: repeating/supporting behavior plus evidence of a
        // different stronger foreground candidate. The absence of foreground
        // evidence on this part is never treated as positive accompaniment data.
        {
            std::vector<part_role_evidence> evidence;
            const auto repetition = role_signal_strength_if_usable(descriptor.rhythmic_repetition);
            const auto sustained = role_signal_strength_if_usable(descriptor.sustained_texture);
            if (repetition.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::rhythmic_ostinato,
                    *repetition,
                    "repeating material supplies a stable support pattern",
                    source,
                    descriptor.part_id));
            }
            if (sustained.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::sustained_texture,
                    *sustained,
                    "sustained material supplies background support",
                    source,
                    descriptor.part_id));
            }
            if (strongest_foreground_part != 0 && strongest_foreground_part != descriptor.part_id &&
                descriptor.auditory_salience.has_value()) {
                const auto& salience = *descriptor.auditory_salience;
                const double background_support = (1.0 - salience.value) * salience.confidence;
                if (background_support >= role_signal_use_threshold) {
                    evidence.push_back(automatic_role_signal_evidence(
                        part_role_evidence_kind::auditory_salience,
                        background_support,
                        "another part carries stronger foreground evidence while this part remains less salient",
                        source,
                        descriptor.part_id,
                        part_role_evidence_origin::auditory_analysis));
                }
            }
            if ((repetition.has_value() || sustained.has_value()) && evidence.size() >= 2)
                append_auto_role_candidate(result, descriptor, musical_part_role::accompaniment, std::move(evidence));
        }

        // Percussion pulse requires actual percussion identity. High activity or
        // rhythmic repetition never turns a pitched part into percussion.
        {
            std::vector<part_role_evidence> evidence;
            const auto percussion = role_signal_strength_if_usable(descriptor.percussion_identity);
            const auto repetition = role_signal_strength_if_usable(descriptor.rhythmic_repetition);
            if (percussion.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::percussion_event_identity,
                    *percussion,
                    "source/performance analysis identifies percussion-event behavior",
                    source,
                    descriptor.part_id));
            }
            if (repetition.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::rhythmic_ostinato,
                    *repetition,
                    "percussion events form a recurring pulse pattern",
                    source,
                    descriptor.part_id));
            }
            if (percussion.has_value())
                append_auto_role_candidate(result, descriptor, musical_part_role::percussion_pulse, std::move(evidence));
        }

        // Doubling support requires explicit correspondence to another musical
        // line. Similar register/timbre by themselves do not establish doubling.
        {
            std::vector<part_role_evidence> evidence;
            const auto doubling = role_signal_strength_if_usable(descriptor.doubling_correspondence);
            if (doubling.has_value()) {
                evidence.push_back(automatic_role_signal_evidence(
                    part_role_evidence_kind::doubling_correspondence,
                    *doubling,
                    "part is independently aligned as a doubling of another persistent line",
                    source,
                    descriptor.part_id));
                if (descriptor.auditory_salience.has_value()) {
                    const auto salience = role_signal_strength_if_usable(descriptor.auditory_salience);
                    if (salience.has_value()) {
                        evidence.push_back(automatic_role_signal_evidence(
                            part_role_evidence_kind::auditory_salience,
                            *salience,
                            "auditory evidence confirms the doubled line is materially present",
                            source,
                            descriptor.part_id,
                            part_role_evidence_origin::auditory_analysis));
                    }
                }
                append_auto_role_candidate(result, descriptor, musical_part_role::doubling_support, std::move(evidence));
            }
        }
    }

    std::sort(result.candidates.begin(), result.candidates.end(), [](const auto& first, const auto& second) {
        if (first.part_id != second.part_id)
            return first.part_id < second.part_id;
        if (first.hypothesis.confidence != second.hypothesis.confidence)
            return first.hypothesis.confidence > second.hypothesis.confidence;
        return static_cast<std::uint8_t>(first.role) < static_cast<std::uint8_t>(second.role);
    });
    return result;
}

} // namespace vgmtooling::model
