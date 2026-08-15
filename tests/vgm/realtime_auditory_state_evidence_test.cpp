#include "../../model/realtime_auditory_state_evidence.h"

#include <cassert>

int main()
{
    using namespace vgmtooling::model;

    // Missing evidence is not silently rewritten as a zero-valued observation.
    const auto missing = normalized_auditory_evidence(false, 0.9f, 0.9f);
    assert(!missing.available);
    assert(missing.score == 0.0f);
    assert(missing.confidence == 0.0f);

    const auto bounded = normalized_auditory_evidence(true, 1.4f, -0.2f);
    assert(bounded.available);
    assert(bounded.score == 1.0f);
    assert(bounded.confidence == 0.0f);

    // Re-identification must preserve ambiguity. Two plausible auditory-object
    // returns are allowed to coexist; the container exposes no automatic winner.
    realtime_identity_hypothesis_set<3> candidates{};
    assert(candidates.add({
        realtime_identity_hypothesis_domain::auditory_object,
        10,
        0,
        0.80f,
        0.70f,
        auditory_continuity_cue_mask(realtime_auditory_continuity_cue::spectral_relation),
    }));
    assert(candidates.add({
        realtime_identity_hypothesis_domain::auditory_object,
        11,
        0,
        0.78f,
        0.68f,
        auditory_continuity_cue_mask(realtime_auditory_continuity_cue::onset_relation),
    }));
    assert(candidates.size() == 2);

    // Repeated evidence for the same candidate merges cues instead of adding a
    // duplicate or deleting its competitor.
    assert(candidates.add({
        realtime_identity_hypothesis_domain::auditory_object,
        10,
        0,
        0.90f,
        0.80f,
        auditory_continuity_cue_mask(realtime_auditory_continuity_cue::phase_trajectory),
    }));
    assert(candidates.size() == 2);
    assert(candidates[0].support == 0.90f);
    assert(candidates[0].confidence == 0.80f);
    assert((candidates[0].cues & auditory_continuity_cue_mask(
        realtime_auditory_continuity_cue::spectral_relation)) != 0);
    assert((candidates[0].cues & auditory_continuity_cue_mask(
        realtime_auditory_continuity_cue::phase_trajectory)) != 0);

    // Source-semantic identity remains a different hypothesis domain from an
    // auditory object even when numeric IDs happen to match.
    assert(candidates.add({
        realtime_identity_hypothesis_domain::source_episode,
        10,
        1,
        1.0f,
        1.0f,
        auditory_continuity_cue_mask(realtime_auditory_continuity_cue::source_semantic),
    }));
    assert(candidates.size() == 3);

    // Fixed capacity fails closed rather than evicting an ambiguity silently.
    assert(!candidates.add({
        realtime_identity_hypothesis_domain::auditory_object,
        12,
        0,
        0.50f,
        0.50f,
        auditory_continuity_cue_mask(realtime_auditory_continuity_cue::level_trajectory),
    }));
    assert(candidates.size() == 3);

    // Continuity confidence and durable-memory plasticity are separate clocks.
    realtime_auditory_state_confidence state{};
    state.continuity = 0.90f;
    state.plasticity = 0.40f;
    assert(!may_update_durable_auditory_memory(state));
    state.plasticity = 0.80f;
    assert(may_update_durable_auditory_memory(state));

    // Strong precision alone is not permission to rewrite durable identity or
    // role memory when continuity ownership is weak.
    state.precision = 1.0f;
    state.continuity = 0.30f;
    assert(!may_update_durable_auditory_memory(state));

    return 0;
}
