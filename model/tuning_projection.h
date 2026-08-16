#pragma once

#include "harmonic_verticality.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct equal_temperament_model {
    std::uint32_t divisions_per_octave = 12;
    double reference_frequency_hz = 440.0;
    std::int64_t reference_step = 69;
    double confidence = 1.0;
    std::string source;
};

struct equal_temperament_pitch_projection {
    harmonic_verticality source_verticality;
    equal_temperament_model tuning;
    std::vector<std::int64_t> nearest_steps;
    std::vector<double> deviations_cents;
    double max_absolute_deviation_cents = 0.0;
    double confidence = 0.0;
};

inline void validate_equal_temperament_model(const equal_temperament_model& tuning) {
    if (tuning.divisions_per_octave == 0)
        throw std::invalid_argument("equal-temperament model requires at least one division per octave");
    if (!std::isfinite(tuning.reference_frequency_hz) || tuning.reference_frequency_hz <= 0.0)
        throw std::invalid_argument("equal-temperament reference frequency must be finite and positive");
    if (tuning.confidence < 0.0 || tuning.confidence > 1.0)
        throw std::invalid_argument("equal-temperament model confidence must be in [0, 1]");
    if (tuning.source.empty())
        throw std::invalid_argument("equal-temperament model requires provenance source");
}

inline equal_temperament_pitch_projection project_verticality_to_equal_temperament(
    const harmonic_verticality& verticality,
    equal_temperament_model tuning,
    double maximum_deviation_cents = 35.0) {
    validate_equal_temperament_model(tuning);
    if (verticality.frequencies_hz.size() < 2)
        throw std::invalid_argument("pitch projection requires a harmonic verticality");
    if (!std::isfinite(maximum_deviation_cents) || maximum_deviation_cents <= 0.0)
        throw std::invalid_argument("pitch-projection tolerance must be finite and positive");

    equal_temperament_pitch_projection result;
    result.source_verticality = verticality;
    result.tuning = std::move(tuning);
    result.confidence = std::min(verticality.confidence, result.tuning.confidence);
    result.nearest_steps.reserve(verticality.frequencies_hz.size());
    result.deviations_cents.reserve(verticality.frequencies_hz.size());

    const double divisions = static_cast<double>(result.tuning.divisions_per_octave);
    for (double frequency : verticality.frequencies_hz) {
        if (!std::isfinite(frequency) || frequency <= 0.0)
            throw std::invalid_argument("harmonic verticality contains an invalid frequency");
        const double exact_step =
            static_cast<double>(result.tuning.reference_step) +
            divisions * std::log2(frequency / result.tuning.reference_frequency_hz);
        const std::int64_t nearest = static_cast<std::int64_t>(std::llround(exact_step));
        const double step_error = exact_step - static_cast<double>(nearest);
        const double cents = step_error * 1200.0 / divisions;
        result.nearest_steps.push_back(nearest);
        result.deviations_cents.push_back(cents);
        result.max_absolute_deviation_cents = std::max(
            result.max_absolute_deviation_cents,
            std::fabs(cents));
    }

    if (result.max_absolute_deviation_cents > maximum_deviation_cents)
        throw std::invalid_argument("musical pitches do not fit the supplied equal-temperament model within tolerance");

    // Quantization confidence degrades continuously as pitches approach the
    // caller's allowed deviation boundary. The tuning and source confidence
    // ceilings remain independent.
    const double fit = std::max(
        0.0,
        1.0 - result.max_absolute_deviation_cents / maximum_deviation_cents);
    result.confidence = std::min(result.confidence, fit);
    return result;
}

inline std::int64_t positive_mod(std::int64_t value, std::int64_t modulus) {
    if (modulus <= 0)
        throw std::invalid_argument("positive modulo requires a positive modulus");
    const std::int64_t remainder = value % modulus;
    return remainder < 0 ? remainder + modulus : remainder;
}

} // namespace vgmtooling::model
