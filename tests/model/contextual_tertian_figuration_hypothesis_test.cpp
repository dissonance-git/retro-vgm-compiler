#include "model/contextual_tertian_figuration_hypothesis.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1.0e-12;
}

equal_temperament_model tuning_12tet() {
    equal_temperament_model result;
    result.divisions_per_octave = 12;
    result.reference_frequency_hz = 440.0;
    result.reference_step = 69;
    result.confidence = 0.99;
    result.source = "contextual-figuration-test";
    return result;
}

equal_temperament_pitch_projection projection_at(
    std::int64_t tick,
    std::vector<std::int64_t> steps,
    std::vector<node_id> parts,
    double confidence = 0.91) {
    equal_temperament_pitch_projection result;
    result.tuning = tuning_12tet();
    result.nearest_steps = std::move(steps);
    result.confidence = confidence;
    result.source_verticality.observation_time = {
        time_domain::source,
        tick,
        1000,
        0,
    };
    result.source_verticality.role = musical_pitch_role::performed;
    result.source_verticality.part_ids = std::move(parts);
    result.source_verticality.confidence = confidence;
    return result;
}

} // namespace

int main() {
    constexpr node_id bass_part = 101;
    constexpr node_id inner_part = 102;
    constexpr node_id moving_part = 103;
    constexpr node_id upper_part = 104;
    const std::vector<node_id> parts{
        bass_part,
        inner_part,
        upper_part,
        moving_part,
    };

    // The exact layer correctly refuses a four-pitch-class surface. Context can
    // still recover a bounded C-major continuation because one persistent upper
    // part traces C-D-E while the exact triad before and after is C major.
    const auto c_before = projection_at(
        1000,
        {48, 52, 55, 60},
        parts,
        0.93);
    const auto c_with_passing_d = projection_at(
        1100,
        {48, 52, 55, 62},
        parts,
        0.91);
    const auto c_after = projection_at(
        1200,
        {48, 52, 55, 64},
        parts,
        0.92);

    const auto before_exact = infer_tertian_triad_hypotheses(c_before);
    const auto surface_exact = infer_tertian_triad_hypotheses(c_with_passing_d);
    const auto after_exact = infer_tertian_triad_hypotheses(c_after);
    assert(before_exact.size() == 1);
    assert(surface_exact.empty());
    assert(after_exact.size() == 1);
    assert(before_exact.front().root_pitch_class == 0);
    assert(before_exact.front().quality == tertian_triad_quality::major);
    assert(after_exact.front().root_pitch_class == 0);
    assert(after_exact.front().quality == tertian_triad_quality::major);

    const auto passing = infer_contextual_tertian_figuration_hypothesis(
        c_before,
        c_with_passing_d,
        c_after);
    assert(passing.has_value());
    assert(passing->root_pitch_class == 0);
    assert(passing->quality == tertian_triad_quality::major);
    assert(passing->inversion == triad_inversion::root_position);
    assert(passing->figuration_kind == contextual_figuration_kind::passing_tone);
    assert(passing->figuration_part_id == moving_part);
    assert(passing->previous_step == 60);
    assert(passing->figuration_step == 62);
    assert(passing->next_step == 64);
    assert(passing->figuration_pitch_class == 2);
    assert(!passing->displaced_structural_pitch_class.has_value());
    assert(passing->retained_structural_pitch_classes == 3);
    assert(passing->surrounding_exact_triad_grounded);
    assert(passing->bass_remains_structural);
    assert(close_enough(
        passing->confidence,
        contextual_tertian_figuration_confidence_ceiling));

    // A neighboring tone may temporarily displace one chord class. Persistent
    // identity plus E-F-E motion distinguishes the F from a free-floating pitch.
    const auto neighbor_before = projection_at(
        2000,
        {48, 55, 60, 64},
        parts,
        0.92);
    const auto neighbor_surface = projection_at(
        2100,
        {48, 55, 60, 65},
        parts,
        0.90);
    const auto neighbor_after = projection_at(
        2200,
        {48, 55, 60, 64},
        parts,
        0.92);
    assert(infer_tertian_triad_hypotheses(neighbor_surface).empty());
    const auto neighbor = infer_contextual_tertian_figuration_hypothesis(
        neighbor_before,
        neighbor_surface,
        neighbor_after);
    assert(neighbor.has_value());
    assert(neighbor->figuration_kind == contextual_figuration_kind::neighbor_tone);
    assert(neighbor->figuration_pitch_class == 5);
    assert(neighbor->displaced_structural_pitch_class.has_value());
    assert(*neighbor->displaced_structural_pitch_class == 4);
    assert(neighbor->retained_structural_pitch_classes == 2);

    // Context must not erase a real competing exact chord. G-A-G in one part is
    // insufficient to reinterpret an exact A-minor first-inversion sonority as
    // merely decorated C major.
    const auto competing_before = projection_at(
        2300,
        {48, 52, 60, 67},
        parts,
        0.92);
    const auto competing_a_minor = projection_at(
        2400,
        {48, 52, 60, 69},
        parts,
        0.90);
    const auto competing_after = projection_at(
        2500,
        {48, 52, 60, 67},
        parts,
        0.92);
    const auto competing_exact = infer_tertian_triad_hypotheses(competing_a_minor);
    assert(competing_exact.size() == 1);
    assert(competing_exact.front().root_pitch_class == 9);
    assert(competing_exact.front().quality == tertian_triad_quality::minor);
    assert(competing_exact.front().inversion == triad_inversion::first);
    assert(!infer_contextual_tertian_figuration_hypothesis(
        competing_before,
        competing_a_minor,
        competing_after).has_value());

    // Bass patterns are harmonically privileged. Even a clean C-D-E passing
    // motion is not reduced when the extra note itself is the sounding bass.
    const auto bass_before = projection_at(
        3000,
        {36, 48, 52, 55},
        {moving_part, bass_part, inner_part, upper_part},
        0.92);
    const auto passing_bass = projection_at(
        3100,
        {38, 48, 52, 55},
        {moving_part, bass_part, inner_part, upper_part},
        0.90);
    const auto bass_after = projection_at(
        3200,
        {40, 48, 52, 55},
        {moving_part, bass_part, inner_part, upper_part},
        0.92);
    assert(!infer_contextual_tertian_figuration_hypothesis(
        bass_before,
        passing_bass,
        bass_after).has_value());

    // Similar pitch classes are not enough. The extra D must belong to a part
    // whose own pitch trajectory exists before and after the surface event.
    const auto unbound_surface = projection_at(
        1100,
        {48, 52, 55, 62},
        {bass_part, inner_part, upper_part, 999},
        0.90);
    assert(!infer_contextual_tertian_figuration_hypothesis(
        c_before,
        unbound_surface,
        c_after).has_value());

    // The surrounding exact harmony must itself agree. Context cannot bridge
    // across a genuine root/quality change merely because the middle surface
    // resembles the earlier sonority.
    const auto f_after = projection_at(
        1200,
        {41, 48, 53, 57},
        parts,
        0.92);
    const auto f_exact = infer_tertian_triad_hypotheses(f_after);
    assert(f_exact.size() == 1);
    assert(f_exact.front().root_pitch_class == 5);
    assert(f_exact.front().quality == tertian_triad_quality::major);
    assert(!infer_contextual_tertian_figuration_hypothesis(
        c_before,
        c_with_passing_d,
        f_after).has_value());

    return 0;
}
