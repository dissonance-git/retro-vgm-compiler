#include "model/analysis_feature.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

struct realization_signature {
    std::string id;
    int patch_family_count = 0;
    int modulation_family_count = 0;
    int active_voice_family_count = 0;
    std::vector<int> transition_pattern;
};

bool same_cheap_local_summary(
    const realization_signature& lhs,
    const realization_signature& rhs) {
    return lhs.patch_family_count == rhs.patch_family_count &&
           lhs.modulation_family_count == rhs.modulation_family_count &&
           lhs.active_voice_family_count == rhs.active_voice_family_count;
}

std::size_t relational_mismatch_count(
    const realization_signature& lhs,
    const realization_signature& rhs) {
    const std::size_t common = lhs.transition_pattern.size() < rhs.transition_pattern.size()
        ? lhs.transition_pattern.size()
        : rhs.transition_pattern.size();

    std::size_t mismatches =
        lhs.transition_pattern.size() > rhs.transition_pattern.size()
            ? lhs.transition_pattern.size() - rhs.transition_pattern.size()
            : rhs.transition_pattern.size() - lhs.transition_pattern.size();

    for (std::size_t i = 0; i < common; ++i) {
        if (lhs.transition_pattern[i] != rhs.transition_pattern[i])
            ++mismatches;
    }
    return mismatches;
}

} // namespace

int main() {
    // Frozen known-answer control for one arrangement / sound-programming
    // realization family. The exact numbers are synthetic. The regression is
    // about test design: a candidate and a decoy are deliberately tied on
    // cheap local summaries before a relational discriminator is allowed to
    // separate them.
    const realization_signature known_control{
        "known-control-A",
        8,
        4,
        6,
        {3, 1, 4, 1, 5, 9},
    };

    const realization_signature held_out_candidate{
        "held-out-A",
        8,
        4,
        6,
        {3, 1, 4, 1, 5, 9},
    };

    const realization_signature matched_decoy{
        "matched-decoy-B",
        8,
        4,
        6,
        {3, 1, 1, 4, 5, 9},
    };

    // Cheap summaries cannot discover the answer because the decoy was built
    // to match them exactly.
    assert(same_cheap_local_summary(known_control, held_out_candidate));
    assert(same_cheap_local_summary(known_control, matched_decoy));

    // A relation over the ordered behavior can distinguish the held-out
    // candidate from the matched decoy. This mirrors the general Helix rule:
    // mask trivial local cues before claiming a fingerprint was discovered.
    assert(relational_mismatch_count(known_control, held_out_candidate) == 0);
    assert(relational_mismatch_count(known_control, matched_decoy) > 0);

    analysis_feature_set attribution;

    auto realization_candidate = present_feature(
        "arrangement_sound_programming_attribution_candidate",
        semantic_layer::musicological_context,
        attribute_value{std::string{"implementer-A"}},
        evidence_status::hypothesis,
        0.90);
    realization_candidate.provenance.push_back({
        evidence_status::hypothesis,
        0.90,
        "matched-decoy-held-out-control",
        std::nullopt,
        "cheap patch/modulation/voice summaries were tied by construction; only the frozen relational transition pattern separated the held-out candidate from the decoy",
    });
    attribution.add(std::move(realization_candidate));

    // Even a successful blinded realization-family discovery is scoped to the
    // realization role. Composer attribution remains unresolved without
    // independent composition or documentary evidence.
    attribution.add(unresolved_feature(
        "composer_attribution",
        semantic_layer::musicological_context,
        feature_availability::unknown,
        "matched-decoy realization fingerprint does not establish composition authorship",
        "attribution-discovery-role-boundary"));

    assert(attribution.find("arrangement_sound_programming_attribution_candidate") != nullptr);
    assert(attribution.find("arrangement_sound_programming_attribution_candidate")->status ==
           evidence_status::hypothesis);
    assert(attribution.find("composer_attribution")->availability == feature_availability::unknown);

    return 0;
}
