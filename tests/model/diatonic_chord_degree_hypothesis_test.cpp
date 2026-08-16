#include "model/diatonic_chord_degree_hypothesis.h"

#include <cmath>
#include <stdexcept>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

equal_temperament_model tuning(double reference_hz = 440.0) {
    equal_temperament_model result;
    result.divisions_per_octave = 12;
    result.reference_frequency_hz = reference_hz;
    result.reference_step = 69;
    result.confidence = 1.0;
    result.source = "test 12-TET";
    return result;
}

tonal_key_class_hypothesis c_ionian_key() {
    tonal_key_class_hypothesis key;
    key.region = time_span{at(0), at(1000)};
    key.tuning = tuning();
    key.pitch_role = musical_pitch_role::performed;
    key.center_octave_class = equal_temperament_step_octave_class(key.tuning, 0);
    key.center_pitch_class = 0;
    key.mode = diatonic_mode::ionian;
    key.collection_scope = pitch_class_collection_scope::structural_hypothesis;
    key.center_cross_origin_grounded = true;
    key.key_class_resolved = true;
    key.confidence = 0.82;
    return key;
}

tertian_triad_hypothesis triad(
    std::int64_t root,
    tertian_triad_quality quality,
    std::int64_t tick = 500,
    double confidence = 0.88,
    musical_pitch_role role = musical_pitch_role::performed) {
    tertian_triad_hypothesis chord;
    chord.root_pitch_class = positive_mod(root, 12);
    chord.quality = quality;
    chord.inversion = triad_inversion::root_position;
    chord.confidence = confidence;
    chord.root_ambiguous = false;
    for (std::int64_t offset : triad_template(quality))
        chord.pitch_classes.push_back(positive_mod(root + offset, 12));
    chord.projection.tuning = tuning();
    chord.projection.source_verticality.observation_time = at(tick);
    chord.projection.source_verticality.role = role;
    chord.projection.confidence = confidence;
    return chord;
}
} // namespace

int main() {
    const auto key = c_ionian_key();

    // G major is structurally degree 5 in C Ionian. That fact still does not
    // establish a Roman numeral or dominant function.
    const auto degree_five = infer_diatonic_chord_degree_hypothesis(
        key,
        triad(7, tertian_triad_quality::major));
    CHECK(degree_five.root_in_diatonic_collection);
    CHECK(degree_five.scale_degree.has_value());
    CHECK(*degree_five.scale_degree == 5);
    CHECK(degree_five.expected_diatonic_quality.has_value());
    CHECK(*degree_five.expected_diatonic_quality == tertian_triad_quality::major);
    CHECK(degree_five.quality_matches_diatonic_stack);
    CHECK(!degree_five.chromatic_root);
    CHECK(close_enough(degree_five.confidence, 0.82));
    CHECK(!degree_five.roman_numeral_named);
    CHECK(!degree_five.tonal_function_named);

    // The leading-tone triad is degree 7 and diminished in Ionian.
    const auto degree_seven = infer_diatonic_chord_degree_hypothesis(
        key,
        triad(11, tertian_triad_quality::diminished));
    CHECK(degree_seven.scale_degree.has_value());
    CHECK(*degree_seven.scale_degree == 7);
    CHECK(degree_seven.expected_diatonic_quality.has_value());
    CHECK(*degree_seven.expected_diatonic_quality == tertian_triad_quality::diminished);
    CHECK(degree_seven.quality_matches_diatonic_stack);

    // D major has a diatonic root but chromatically altered quality in C Ionian.
    // This layer must not leap from that observation to V/V or secondary-dominant
    // function.
    const auto altered_two = infer_diatonic_chord_degree_hypothesis(
        key,
        triad(2, tertian_triad_quality::major));
    CHECK(altered_two.scale_degree.has_value());
    CHECK(*altered_two.scale_degree == 2);
    CHECK(altered_two.expected_diatonic_quality.has_value());
    CHECK(*altered_two.expected_diatonic_quality == tertian_triad_quality::minor);
    CHECK(!altered_two.quality_matches_diatonic_stack);
    CHECK(!altered_two.chromatic_root);
    CHECK(!altered_two.roman_numeral_named);
    CHECK(!altered_two.tonal_function_named);

    // B-flat is outside the C-Ionian collection. It receives no invented degree.
    const auto flat_seven_root = infer_diatonic_chord_degree_hypothesis(
        key,
        triad(10, tertian_triad_quality::major));
    CHECK(!flat_seven_root.root_in_diatonic_collection);
    CHECK(!flat_seven_root.scale_degree.has_value());
    CHECK(flat_seven_root.chromatic_root);
    CHECK(!flat_seven_root.roman_numeral_named);
    CHECK(!flat_seven_root.tonal_function_named);

    bool unresolved_key_rejected = false;
    try {
        auto unresolved = key;
        unresolved.key_class_resolved = false;
        unresolved.mode.reset();
        (void)infer_diatonic_chord_degree_hypothesis(
            unresolved,
            triad(7, tertian_triad_quality::major));
    } catch (const std::invalid_argument&) {
        unresolved_key_rejected = true;
    }
    CHECK(unresolved_key_rejected);

    bool tuning_mismatch_rejected = false;
    try {
        auto wrong_tuning = triad(7, tertian_triad_quality::major);
        wrong_tuning.projection.tuning = tuning(442.0);
        (void)infer_diatonic_chord_degree_hypothesis(key, wrong_tuning);
    } catch (const std::invalid_argument&) {
        tuning_mismatch_rejected = true;
    }
    CHECK(tuning_mismatch_rejected);

    bool pitch_role_mismatch_rejected = false;
    try {
        const auto heard = triad(
            7,
            tertian_triad_quality::major,
            500,
            0.88,
            musical_pitch_role::heard);
        (void)infer_diatonic_chord_degree_hypothesis(key, heard);
    } catch (const std::invalid_argument&) {
        pitch_role_mismatch_rejected = true;
    }
    CHECK(pitch_role_mismatch_rejected);

    bool ambiguous_root_rejected = false;
    try {
        auto ambiguous = triad(7, tertian_triad_quality::major);
        ambiguous.root_ambiguous = true;
        (void)infer_diatonic_chord_degree_hypothesis(key, ambiguous);
    } catch (const std::invalid_argument&) {
        ambiguous_root_rejected = true;
    }
    CHECK(ambiguous_root_rejected);

    bool outside_region_rejected = false;
    try {
        const auto outside = triad(7, tertian_triad_quality::major, 1000);
        (void)infer_diatonic_chord_degree_hypothesis(key, outside);
    } catch (const std::invalid_argument&) {
        outside_region_rejected = true;
    }
    CHECK(outside_region_rejected);

    // Materialization requires both support nodes before it mutates the graph.
    musical_execution_graph graph;
    node key_node;
    key_node.kind = node_kind::pattern;
    key_node.layer = semantic_layer::musical_structure;
    key_node.flow = flow_kind::value;
    key_node.label = "test key";
    const node_id key_id = graph.add_node(std::move(key_node));

    node chord_node;
    chord_node.kind = node_kind::pattern;
    chord_node.layer = semantic_layer::musical_structure;
    chord_node.flow = flow_kind::value;
    chord_node.label = "test chord";
    const node_id chord_id = graph.add_node(std::move(chord_node));

    const node_id relation_id = add_diatonic_chord_degree_hypothesis(
        graph,
        degree_five,
        key_id,
        chord_id);
    CHECK(graph.find_node(relation_id) != nullptr);
    CHECK(graph.edges().size() == 2);

    bool missing_support_rejected = false;
    try {
        (void)add_diatonic_chord_degree_hypothesis(
            graph,
            degree_five,
            key_id,
            9999);
    } catch (const std::invalid_argument&) {
        missing_support_rejected = true;
    }
    CHECK(missing_support_rejected);
    CHECK(graph.nodes().size() == 3);
    CHECK(graph.edges().size() == 2);

    return 0;
}
