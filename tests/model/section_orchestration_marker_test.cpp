#include "model/section_orchestration_marker.h"

#include <cassert>
#include <cmath>
#include <vector>

using namespace vgmtooling::model;

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 44100, 0};
}

orchestration_transition_hypothesis change(
    orchestration_transition_kind kind,
    node_id part,
    std::int64_t tick,
    double confidence) {
    orchestration_transition_hypothesis result;
    result.kind = kind;
    result.first_part_id = part;
    result.second_part_id = part;
    result.first_role = musical_part_role::accompaniment;
    result.second_role = musical_part_role::accompaniment;
    result.transition_time = at(tick);
    result.persistent_part_preserved = true;
    result.role_preserved = true;
    result.confidence = confidence;
    return result;
}

} // namespace

int main() {
    auto timbre = change(
        orchestration_transition_kind::timbral_recoloring,
        10,
        100,
        0.82);
    timbre.realization_comparable = true;
    timbre.timbre_changed = true;

    auto register_change = change(
        orchestration_transition_kind::registral_revoicing,
        11,
        103,
        0.79);
    register_change.register_comparable = true;
    register_change.register_shift = 0.5;

    auto late = change(
        orchestration_transition_kind::role_change,
        12,
        140,
        0.95);
    late.role_preserved = false;
    late.second_role = musical_part_role::melodic_foreground;

    const auto marker = infer_section_orchestration_marker(
        at(100),
        0.91,
        {timbre, register_change, late},
        5);
    assert(marker.qualifying_transition_count == 2);
    assert(marker.independent_part_count == 2);
    assert(marker.multi_part_grounded);
    assert(marker.timbre_change_count == 1);
    assert(marker.register_change_count == 1);
    assert(marker.role_change_count == 0);
    assert(std::fabs(marker.confidence - 0.79) < 1e-12);

    // A single orchestration change may reinforce an already known boundary,
    // but cannot become strong evidence for a whole-section orchestration event.
    const auto single = infer_section_orchestration_marker(
        at(100),
        0.95,
        {timbre},
        0);
    assert(single.qualifying_transition_count == 1);
    assert(!single.multi_part_grounded);
    assert(std::fabs(single.confidence - single_orchestration_marker_ceiling) < 1e-12);

    // Stable assignments and off-boundary changes do not manufacture a marker.
    auto stable = change(
        orchestration_transition_kind::stable_assignment,
        10,
        100,
        0.99);
    const auto none = infer_section_orchestration_marker(
        at(100),
        0.90,
        {stable, late},
        2);
    assert(none.qualifying_transition_count == 0);
    assert(none.confidence == 0.0);

    // The form boundary is an input to this analysis. Weak boundary evidence
    // remains weak even if many orchestration changes happen nearby.
    const auto weak_boundary = infer_section_orchestration_marker(
        at(100),
        0.42,
        {timbre, register_change},
        5);
    assert(weak_boundary.multi_part_grounded);
    assert(std::fabs(weak_boundary.confidence - 0.42) < 1e-12);

    return 0;
}
