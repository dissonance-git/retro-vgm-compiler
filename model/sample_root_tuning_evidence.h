#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace vgmtooling::model {

// Sample-root tuning is independent evidence about the pitch of one exact
// sample version at unity playback. A runtime sample identity or playback-rate
// register does not imply this value by itself.
enum class sample_root_tuning_origin : std::uint8_t {
    authored_metadata = 0,
    exact_upstream_calibration,
    auditory_fundamental_estimate,
};

enum class sample_root_tuning_state : std::uint8_t {
    pitched = 0,
    unpitched,
    ambiguous,
};

struct sample_root_tuning_evidence {
    node_id sample_version_id = 0;
    sample_root_tuning_origin origin = sample_root_tuning_origin::auditory_fundamental_estimate;
    sample_root_tuning_state state = sample_root_tuning_state::ambiguous;

    // Fundamental frequency of this sample when its source-specific sampler is
    // at unity playback. For the SNES S-DSP bridge, unity means pitch rate
    // 0x1000. It is not a MIDI key number and does not imply harmonic function.
    double unity_playback_fundamental_hz = 0.0;

    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
};

constexpr double sample_root_tuning_origin_confidence_ceiling(
    sample_root_tuning_origin origin) noexcept {
    switch (origin) {
    case sample_root_tuning_origin::authored_metadata:
        return 1.0;
    case sample_root_tuning_origin::exact_upstream_calibration:
        return 0.95;
    case sample_root_tuning_origin::auditory_fundamental_estimate:
        return 0.80;
    }
    return 0.0;
}

inline sample_root_tuning_evidence make_sample_root_tuning_evidence(
    node_id sample_version_id,
    sample_root_tuning_origin origin,
    sample_root_tuning_state state,
    double unity_playback_fundamental_hz,
    evidence_status status,
    double proposed_confidence,
    std::string source) {
    if (sample_version_id == 0)
        throw std::invalid_argument("sample-root tuning requires an exact sample-version id");
    if (source.empty())
        throw std::invalid_argument("sample-root tuning requires provenance source");
    if (!std::isfinite(proposed_confidence) ||
        proposed_confidence < 0.0 || proposed_confidence > 1.0) {
        throw std::invalid_argument("sample-root tuning confidence must be finite in [0, 1]");
    }

    if (state == sample_root_tuning_state::pitched) {
        if (!std::isfinite(unity_playback_fundamental_hz) ||
            unity_playback_fundamental_hz <= 0.0) {
            throw std::invalid_argument(
                "pitched sample-root tuning requires a finite positive unity fundamental");
        }
    } else if (unity_playback_fundamental_hz != 0.0) {
        throw std::invalid_argument(
            "unpitched or ambiguous sample-root tuning cannot carry a preferred fundamental");
    }

    sample_root_tuning_evidence result;
    result.sample_version_id = sample_version_id;
    result.origin = origin;
    result.state = state;
    result.unity_playback_fundamental_hz = unity_playback_fundamental_hz;
    result.status = status;
    result.confidence = std::min(
        proposed_confidence,
        sample_root_tuning_origin_confidence_ceiling(origin));
    result.source = std::move(source);
    return result;
}

inline bool usable_sample_root_tuning(
    const sample_root_tuning_evidence& tuning) noexcept {
    return tuning.sample_version_id != 0 &&
        tuning.state == sample_root_tuning_state::pitched &&
        std::isfinite(tuning.unity_playback_fundamental_hz) &&
        tuning.unity_playback_fundamental_hz > 0.0 &&
        std::isfinite(tuning.confidence) &&
        tuning.confidence > 0.0 && tuning.confidence <= 1.0 &&
        !tuning.source.empty();
}

} // namespace vgmtooling::model
