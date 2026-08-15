#include "../../model/realtime_musical_role_tracker.h"

#include <cassert>
#include <cmath>

namespace {

vgmtooling::model::realtime_musical_role_hypotheses make_roles(
    float foundation_score,
    float foundation_confidence)
{
    vgmtooling::model::realtime_musical_role_hypotheses roles{};
    roles.foundation = {foundation_score, foundation_confidence, 1};
    return roles;
}

} // namespace

int main()
{
    using namespace vgmtooling::model;

    realtime_musical_role_tracker<3> tracker{};
    spatial_source_evidence source{};
    source.source_id = 10;
    source.generation = 1;

    assert(tracker.advance_block(4800, 48000.0));
    tracker.observe(source, make_roles(0.80f, 0.80f));

    realtime_musical_role_hypotheses output{};
    assert(tracker.lookup(source, output));
    assert(output.foundation.score == 0.80f);

    // Multiple sources observed in one audio block share the same musical time.
    // Lane count must never multiply elapsed time.
    spatial_source_evidence same_block_source{};
    same_block_source.source_id = 99;
    tracker.observe(same_block_source, make_roles(0.30f, 0.30f));
    assert(std::fabs(tracker.stream_seconds() - 0.10) < 1.0e-9);

    // The same identity moves gradually toward new evidence after exactly one
    // additional block rather than snapping to the latest block estimate.
    assert(tracker.advance_block(4800, 48000.0));
    tracker.observe(source, make_roles(0.10f, 0.80f));
    assert(tracker.lookup(source, output));
    assert(output.foundation.score < 0.80f);
    assert(output.foundation.score > 0.10f);
    assert(std::fabs(tracker.stream_seconds() - 0.20) < 1.0e-9);

    // A new source generation is a hard continuity boundary unless a stronger
    // persistent-part identity explicitly bridges it.
    spatial_source_evidence new_generation = source;
    new_generation.generation = 2;
    assert(!tracker.lookup(new_generation, output));
    tracker.observe(new_generation, make_roles(0.20f, 0.90f));
    assert(tracker.lookup(new_generation, output));
    assert(output.foundation.score == 0.20f);

    // A sufficiently supported persistent part survives source-episode changes.
    assert(tracker.advance_block(4800, 48000.0));
    spatial_source_evidence part_a{};
    part_a.source_id = 20;
    part_a.generation = 1;
    part_a.persistent_part_present = true;
    part_a.persistent_part_id = 777;
    part_a.persistent_part_confidence = 0.90f;
    tracker.observe(part_a, make_roles(0.90f, 0.90f));

    spatial_source_evidence part_b = part_a;
    part_b.source_id = 21;
    part_b.generation = 5;
    assert(tracker.lookup(part_b, output));
    assert(output.foundation.score == 0.90f);

    // A weak part-ID hypothesis is not enough to merge separate source episodes.
    spatial_source_evidence weak_part_a = part_a;
    weak_part_a.source_id = 30;
    weak_part_a.persistent_part_confidence = 0.50f;
    tracker.observe(weak_part_a, make_roles(0.70f, 0.70f));
    spatial_source_evidence weak_part_b = weak_part_a;
    weak_part_b.source_id = 31;
    assert(!tracker.lookup(weak_part_b, output));

    // Causal time can expire a role identity even while no source is observed.
    realtime_musical_role_tracker<4> expiry_tracker{};
    auto expiry_policy = expiry_tracker.policy();
    expiry_policy.max_hold_seconds = 0.15f;
    assert(expiry_tracker.set_policy(expiry_policy));
    assert(expiry_tracker.advance_block(4800, 48000.0));
    expiry_tracker.observe(source, make_roles(0.80f, 0.80f));
    assert(expiry_tracker.lookup(source, output));
    assert(expiry_tracker.advance_block(9600, 48000.0));
    assert(!expiry_tracker.lookup(source, output));

    // Fixed-capacity eviction reinitializes the selected slot. An incoming
    // source must never blend with the role state of the evicted identity.
    realtime_musical_role_tracker<1> tiny_tracker{};
    assert(tiny_tracker.advance_block(4800, 48000.0));
    tiny_tracker.observe(source, make_roles(0.90f, 0.90f));
    spatial_source_evidence replacement{};
    replacement.source_id = 40;
    tiny_tracker.observe(replacement, make_roles(0.20f, 0.40f));
    assert(!tiny_tracker.lookup(source, output));
    assert(tiny_tracker.lookup(replacement, output));
    assert(output.foundation.score == 0.20f);

    // Invalid policy or clock input fails without advancing stream time.
    auto invalid_policy = tracker.policy();
    invalid_policy.max_hold_seconds = -1.0f;
    assert(!tracker.set_policy(invalid_policy));
    const double before_invalid_clock = tracker.stream_seconds();
    assert(!tracker.advance_block(1, 0.0));
    assert(tracker.stream_seconds() == before_invalid_clock);

    return 0;
}
