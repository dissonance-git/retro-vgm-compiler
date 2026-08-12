#pragma once

#include "musical_execution_graph.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// Availability is separate from evidential strength.
//
// present:
//   the feature has a value for this subject and that value carries its own
//   exact/derived/hypothesis status.
//
// unknown:
//   the feature is meaningful for this subject, but its value has not been
//   established from the available evidence.
//
// unavailable:
//   the current source/adapter does not expose enough evidence to answer the
//   feature question.
//
// not_applicable:
//   the feature has no valid semantics for this subject.
//
// These states must not be collapsed into zero, false, or an empty string.
enum class feature_availability : std::uint8_t {
    present = 0,
    unknown,
    unavailable,
    not_applicable,
};

struct analysis_feature {
    std::string name;
    feature_availability availability = feature_availability::unknown;
    std::optional<attribute_value> value{};
    std::string unit;
    std::optional<evidence_status> status{};
    std::optional<double> confidence{};
    std::vector<provenance_ref> provenance;
    std::vector<node_id> support_nodes;
    std::vector<edge_id> support_edges;
};

inline void validate_analysis_feature(const analysis_feature& feature) {
    if (feature.name.empty())
        throw std::invalid_argument("analysis feature requires a non-empty name");

    if (feature.availability == feature_availability::present) {
        if (!feature.value.has_value())
            throw std::invalid_argument("present analysis feature requires a value");
        if (!feature.status.has_value())
            throw std::invalid_argument("present analysis feature requires evidence status");
        if (!feature.confidence.has_value())
            throw std::invalid_argument("present analysis feature requires confidence");
        if (*feature.confidence < 0.0 || *feature.confidence > 1.0)
            throw std::invalid_argument("analysis feature confidence must be in [0, 1]");
        return;
    }

    if (feature.value.has_value())
        throw std::invalid_argument("non-present analysis feature cannot carry a value");
    if (feature.status.has_value())
        throw std::invalid_argument("non-present analysis feature cannot carry evidence status");
    if (feature.confidence.has_value())
        throw std::invalid_argument("non-present analysis feature cannot carry confidence");
}

class analysis_feature_set {
public:
    void add(analysis_feature feature) {
        validate_analysis_feature(feature);
        if (find(feature.name) != nullptr)
            throw std::invalid_argument("analysis feature set already contains this feature name");
        features_.push_back(std::move(feature));
    }

    const analysis_feature* find(const std::string& name) const noexcept {
        for (const auto& feature : features_) {
            if (feature.name == name)
                return &feature;
        }
        return nullptr;
    }

    const std::vector<analysis_feature>& features() const noexcept { return features_; }

private:
    std::vector<analysis_feature> features_;
};

inline analysis_feature present_feature(
    std::string name,
    attribute_value value,
    evidence_status status,
    double confidence,
    std::string unit = {}) {
    analysis_feature feature;
    feature.name = std::move(name);
    feature.availability = feature_availability::present;
    feature.value = std::move(value);
    feature.unit = std::move(unit);
    feature.status = status;
    feature.confidence = confidence;
    validate_analysis_feature(feature);
    return feature;
}

inline analysis_feature unresolved_feature(
    std::string name,
    feature_availability availability,
    std::string detail = {},
    std::string source = {}) {
    if (availability == feature_availability::present)
        throw std::invalid_argument("unresolved analysis feature cannot be present");

    analysis_feature feature;
    feature.name = std::move(name);
    feature.availability = availability;
    if (!detail.empty() || !source.empty()) {
        feature.provenance.push_back({
            evidence_status::exact,
            1.0,
            std::move(source),
            std::nullopt,
            std::move(detail),
        });
    }
    validate_analysis_feature(feature);
    return feature;
}

} // namespace vgmtooling::model
