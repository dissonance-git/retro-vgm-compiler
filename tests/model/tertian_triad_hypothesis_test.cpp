#include "model/contextual_tertian_figuration_hypothesis.h"
#include "model/tertian_triad_hypothesis.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

double frequency_for_step(std::int64_t step) {
    return 440.0 * std::pow(2.0, (static_cast<double>(step) - 69.0) / 12.0);
}

node_id add_part(musical_execution_graph& graph, const char* label) {
    node value;
    value.kind = node_kind::part;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::stream;
    value.label = label;
    return graph.add_node(std::move(value));
}

node_id add_event(musical_execution_graph& graph, const char* label) {
    node value;
    value.kind = node_kind::musical_event;
    value.layer = semantic_layer::musical_performance;
    value.flow = flow_kind::stream;
    value.label = label;
    value.active = time_span{
        {time_domain::musical, 0, 960, 0},
        time_coordinate{time_domain::musical, 960, 960, 0},
    };
    return graph.add_node(std::move(value));
}

absolute_musical_pitch_observation pitch(
    node_id event,
    node_id part,
    std::int64_t step,
    double confidence = 0.95) {
    return {
        event,
        part,
        {
            {time_domain::musical, 0, 960, 0},
            time_coordinate{time_domain::musical, 960, 960, 0},
        },
        frequency_for_step(step),
        musical_pitch_role::programmed,
        evidence_status::derived,
        confidence,
        "synthetic-programmed-pitch",
    };
}

const attribute* find_attribute(const node& value, const char* name) {
    for (const auto& item : value.attributes) {
        if (item.name == name)
            return &item;
    }
    return nullptr;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

equal_temperament_pitch_projection contextual_projection(
    std::int64_t tick,
    std::vector<std::int64_t> steps,
    std::vector<node_id> parts,
    const equal_temperament_model& tuning,
    double confidence = 0.91) {
    equal_temperament_pitch_projection result;
    result.tuning = tuning;
    result.nearest_steps = std::move(steps);
    result.confidence = confidence;
    result.source_verticality.observation_time = {
        time_domain::musical,
        tick,
        960,
        0,
    };
    result.source_verticality.role = musical_pitch_role::performed;
    result.source_verticality.part_ids = std::move(parts);
    result.source_verticality.confidence = confidence;
    return result;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id bass = add_part(graph, "bass");
    const node_id inner = add_part(graph, "inner");
    const node_id melody = add_part(graph, "melody");
    const node_id e1 = add_event(graph, "A");
    const node_id e2 = add_event(graph, "C sharp");
    const node_id e3 = add_event(graph, "E");

    const auto verticality = make_harmonic_verticality(
        {time_domain::musical, 480, 960, 0},
        {
            pitch(e1, bass, 45, 0.97),
            pitch(e2, inner, 49, 0.95),
            pitch(e3, melody, 52, 0.93),
        });

    equal_temperament_model tuning;
    tuning.divisions_per_octave = 12;
    tuning.reference_frequency_hz = 440.0;
    tuning.reference_step = 69;
    tuning.confidence = 0.99;
    tuning.source = "explicit-12tet-control";

    const auto projection = project_verticality_to_equal_temperament(
        verticality,
        tuning,
        20.0);
    assert(projection.nearest_steps.size() == 3);
    assert(projection.nearest_steps[0] == 45);
    assert(projection.nearest_steps[1] == 49);
    assert(projection.nearest_steps[2] == 52);
    assert(close_enough(projection.max_absolute_deviation_cents, 0.0));
    assert(close_enough(projection.confidence, 0.93));

    const auto candidates = infer_tertian_triad_hypotheses(projection);
    assert(candidates.size() == 1);
    const auto& major = candidates.front();
    assert(major.root_pitch_class == 9);
    assert(major.quality == tertian_triad_quality::major);
    assert(major.inversion == triad_inversion::root_position);
    assert(!major.root_ambiguous);
    assert(close_enough(major.confidence, 0.93));

    const node_id chord = add_tertian_triad_hypothesis(graph, major);
    const node* materialized = graph.find_node(chord);
    assert(materialized != nullptr);
    assert(materialized->kind == node_kind::pattern);
    assert(std::get<std::string>(find_attribute(*materialized, "quality")->value) == "major");
    assert(std::get<std::int64_t>(find_attribute(*materialized, "root_pitch_class")->value) == 9);
    assert(find_attribute(*materialized, "key") == nullptr);
    assert(find_attribute(*materialized, "harmonic_function") == nullptr);
    assert(find_attribute(*materialized, "enharmonic_spelling") == nullptr);

    // First inversion is a voicing fact once the root candidate is known.
    const node_id f1 = add_event(graph, "C sharp bass");
    const node_id f2 = add_event(graph, "E upper");
    const node_id f3 = add_event(graph, "A upper");
    const auto first_inversion_projection = project_verticality_to_equal_temperament(
        make_harmonic_verticality(
            {time_domain::musical, 480, 960, 0},
            {
                pitch(f1, bass, 49),
                pitch(f2, inner, 52),
                pitch(f3, melody, 57),
            }),
        tuning,
        20.0);
    const auto first_inversion = infer_tertian_triad_hypotheses(first_inversion_projection);
    assert(first_inversion.size() == 1);
    assert(first_inversion.front().root_pitch_class == 9);
    assert(first_inversion.front().quality == tertian_triad_quality::major);
    assert(first_inversion.front().inversion == triad_inversion::first);

    // Augmented triads are symmetrical. Do not invent one privileged root from
    // the pitch-class set alone.
    const node_id a1 = add_event(graph, "aug1");
    const node_id a2 = add_event(graph, "aug2");
    const node_id a3 = add_event(graph, "aug3");
    const auto augmented_projection = project_verticality_to_equal_temperament(
        make_harmonic_verticality(
            {time_domain::musical, 480, 960, 0},
            {
                pitch(a1, bass, 60),
                pitch(a2, inner, 64),
                pitch(a3, melody, 68),
            }),
        tuning,
        20.0);
    const auto augmented = infer_tertian_triad_hypotheses(augmented_projection);
    assert(augmented.size() == 3);
    for (const auto& candidate : augmented) {
        assert(candidate.quality == tertian_triad_quality::augmented);
        assert(candidate.root_ambiguous);
        assert(candidate.confidence <= ambiguous_triad_root_ceiling);
    }

    // The current triad vocabulary is explicitly 12-TET only.
    equal_temperament_model nineteen = tuning;
    nineteen.divisions_per_octave = 19;
    const auto nineteen_projection = project_verticality_to_equal_temperament(
        verticality,
        nineteen,
        50.0);
    bool rejected_nineteen = false;
    try {
        (void)infer_tertian_triad_hypotheses(nineteen_projection);
    } catch (const std::invalid_argument&) {
        rejected_nineteen = true;
    }
    assert(rejected_nineteen);

    // A frequency outside the declared tuning tolerance must be rejected rather
    // than rounded into the nearest chord tone.
    auto detuned = verticality;
    detuned.frequencies_hz[1] *= std::pow(2.0, 50.0 / 1200.0);
    bool rejected_detuned = false;
    try {
        (void)project_verticality_to_equal_temperament(detuned, tuning, 20.0);
    } catch (const std::invalid_argument&) {
        rejected_detuned = true;
    }
    assert(rejected_detuned);

    // Contextual figuration is a deliberately weaker layer above exact triad
    // inference. A persistent upper part moves C-D-E while C/E/G remains the
    // surrounding exact harmony. The four-class surface has no exact triad, but
    // the linear part motion and matching context can support a bounded C-major
    // continuation hypothesis without deleting the observed D.
    constexpr node_id context_bass = 101;
    constexpr node_id context_inner = 102;
    constexpr node_id moving_part = 103;
    constexpr node_id context_upper = 104;
    const std::vector<node_id> context_parts{
        context_bass,
        context_inner,
        context_upper,
        moving_part,
    };
    const auto c_before = contextual_projection(
        1000,
        {48, 52, 55, 60},
        context_parts,
        tuning,
        0.93);
    const auto c_with_passing_d = contextual_projection(
        1100,
        {48, 52, 55, 62},
        context_parts,
        tuning,
        0.91);
    const auto c_after = contextual_projection(
        1200,
        {48, 52, 55, 64},
        context_parts,
        tuning,
        0.92);
    assert(infer_tertian_triad_hypotheses(c_before).size() == 1);
    assert(infer_tertian_triad_hypotheses(c_with_passing_d).empty());
    assert(infer_tertian_triad_hypotheses(c_after).size() == 1);
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

    // A true neighbor can temporarily displace one structural chord class. The
    // same persistent part moves E-F-E while C and G remain. This is weaker than
    // an exact triad but still traceable to a specific performed line.
    const auto neighbor_before = contextual_projection(
        2000,
        {48, 55, 60, 64},
        context_parts,
        tuning,
        0.92);
    const auto neighbor_surface = contextual_projection(
        2100,
        {48, 55, 60, 65},
        context_parts,
        tuning,
        0.90);
    const auto neighbor_after = contextual_projection(
        2200,
        {48, 55, 60, 64},
        context_parts,
        tuning,
        0.92);
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

    // Context is not allowed to erase a competing exact chord. C major around an
    // exact A-minor first-inversion sonority remains a real harmonic alternative,
    // even if one persistent part happens to trace G-A-G.
    const auto competing_exact = contextual_projection(
        2300,
        {48, 52, 60, 69},
        context_parts,
        tuning,
        0.90);
    const auto exact_candidates = infer_tertian_triad_hypotheses(competing_exact);
    assert(exact_candidates.size() == 1);
    assert(exact_candidates.front().root_pitch_class == 9);
    assert(exact_candidates.front().quality == tertian_triad_quality::minor);
    assert(!infer_contextual_tertian_figuration_hypothesis(
        neighbor_before,
        competing_exact,
        neighbor_after).has_value());

    // Bass motion carries disproportionate harmonic information. Even a clean
    // C-D-E passing line is not reduced away when the extra pitch is the bass.
    const auto bass_before = contextual_projection(
        3000,
        {36, 48, 52, 55},
        {moving_part, context_bass, context_inner, context_upper},
        tuning,
        0.92);
    const auto passing_bass = contextual_projection(
        3100,
        {38, 48, 52, 55},
        {moving_part, context_bass, context_inner, context_upper},
        tuning,
        0.90);
    const auto bass_after = contextual_projection(
        3200,
        {40, 48, 52, 55},
        {moving_part, context_bass, context_inner, context_upper},
        tuning,
        0.92);
    assert(!infer_contextual_tertian_figuration_hypothesis(
        bass_before,
        passing_bass,
        bass_after).has_value());

    // Pitch-class resemblance without persistent-line continuity is insufficient.
    // A transient D introduced by a new part cannot borrow C-major context from
    // unrelated surrounding voices.
    const auto unbound_surface = contextual_projection(
        4100,
        {48, 52, 55, 62},
        {context_bass, context_inner, context_upper, 999},
        tuning,
        0.90);
    assert(!infer_contextual_tertian_figuration_hypothesis(
        c_before,
        unbound_surface,
        c_after).has_value());

    // Surrounding harmony must actually agree. Similar local pitch material
    // across a harmonic change is not enough to manufacture a stable triad.
    const auto f_after = contextual_projection(
        4200,
        {41, 48, 53, 57},
        context_parts,
        tuning,
        0.92);
    assert(infer_tertian_triad_hypotheses(f_after).size() == 1);
    assert(!infer_contextual_tertian_figuration_hypothesis(
        c_before,
        c_with_passing_d,
        f_after).has_value());

    return 0;
}
