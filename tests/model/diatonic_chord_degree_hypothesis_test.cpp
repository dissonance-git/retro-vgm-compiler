#include "model/cadential_degree_relation_hypothesis.h"
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

cadential_arrival_hypothesis arrival_between(
    const tertian_triad_hypothesis& first,
    const tertian_triad_hypothesis& second,
    bool cross_part_phrase_grounded = true,
    bool reliable_root_motion = true,
    bool with_identity_grounded_voice_leading = true) {
    phrase_boundary_consensus boundary;
    boundary.representative = second.projection.source_verticality.observation_time;
    boundary.confidence = 0.90;
    boundary.cross_part_grounded = cross_part_phrase_grounded;

    auto transition = infer_harmonic_transition(first, second);
    transition.confidence = 0.91;
    transition.root_motion_reliable = reliable_root_motion;

    if (!with_identity_grounded_voice_leading)
        return infer_cadential_arrival(boundary, transition);

    voice_leading_hypothesis voices;
    voices.first_time = first.projection.source_verticality.observation_time;
    voices.second_time = second.projection.source_verticality.observation_time;
    voices.total_absolute_motion_semitones = 4;
    voices.all_correspondence_identity_grounded = true;
    voices.confidence = 0.88;
    return infer_cadential_arrival(boundary, transition, voices);
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

    // Key-relative degree motion is useful evidence at a phrase arrival, but it
    // remains one layer below functional Roman-numeral/cadence naming. G -> C
    // in C Ionian is therefore "five_to_one_arrival", not automatically PAC/IAC.
    const auto g_major_100 = triad(7, tertian_triad_quality::major, 100);
    const auto c_major_200 = triad(0, tertian_triad_quality::major, 200);
    const auto five_to_one_arrival = arrival_between(g_major_100, c_major_200);
    const auto five_to_one = infer_cadential_degree_relation(
        key,
        g_major_100,
        c_major_200,
        five_to_one_arrival);
    CHECK(five_to_one.kind == cadential_degree_relation_kind::five_to_one_arrival);
    CHECK(five_to_one.first_scale_degree.has_value());
    CHECK(*five_to_one.first_scale_degree == 5);
    CHECK(five_to_one.arrival_scale_degree.has_value());
    CHECK(*five_to_one.arrival_scale_degree == 1);
    CHECK(five_to_one.root_motion_semitones == 5);
    CHECK(five_to_one.root_interval_class == 5);
    CHECK(five_to_one.phrase_grounded);
    CHECK(five_to_one.root_motion_grounded);
    CHECK(five_to_one.voice_leading_grounded);
    CHECK(five_to_one.first_time.tick == 100);
    CHECK(five_to_one.arrival_time.tick == 200);
    CHECK(five_to_one_arrival.departure_time.tick == 100);
    CHECK(five_to_one_arrival.departure_root_pitch_class == 7);
    CHECK(five_to_one_arrival.arrival_root_pitch_class == 0);
    CHECK(five_to_one_arrival.departure_quality == tertian_triad_quality::major);
    CHECK(five_to_one_arrival.arrival_quality == tertian_triad_quality::major);
    CHECK(!five_to_one_arrival.quality_changed);
    CHECK(close_enough(five_to_one.confidence, 0.82));
    CHECK(!five_to_one.roman_numeral_named);
    CHECK(!five_to_one.tonal_function_named);
    CHECK(!five_to_one.cadence_class_named);

    // The same degree-5 departure can arrive on degree 6. Record that sequential
    // fact without inventing the label "deceptive cadence" at this layer.
    const auto g_major_300 = triad(7, tertian_triad_quality::major, 300);
    const auto a_minor_400 = triad(9, tertian_triad_quality::minor, 400);
    const auto five_to_six_arrival = arrival_between(g_major_300, a_minor_400);
    const auto five_to_six = infer_cadential_degree_relation(
        key,
        g_major_300,
        a_minor_400,
        five_to_six_arrival);
    CHECK(five_to_six.kind == cadential_degree_relation_kind::five_to_six_arrival);
    CHECK(five_to_six_arrival.quality_changed);
    CHECK(five_to_six_arrival.departure_quality == tertian_triad_quality::major);
    CHECK(five_to_six_arrival.arrival_quality == tertian_triad_quality::minor);
    CHECK(!five_to_six.cadence_class_named);
    CHECK(!five_to_six.tonal_function_named);

    // Arrival on degree 5 is separately represented, but does not become a half
    // cadence unless a later layer earns that interpretation from more context.
    const auto d_minor_500 = triad(2, tertian_triad_quality::minor, 500);
    const auto g_major_600 = triad(7, tertian_triad_quality::major, 600);
    const auto on_five = infer_cadential_degree_relation(
        key,
        d_minor_500,
        g_major_600,
        arrival_between(d_minor_500, g_major_600));
    CHECK(on_five.kind == cadential_degree_relation_kind::arrival_on_five);
    CHECK(!on_five.cadence_class_named);

    // Phrase grounding is independent evidence. A root relation occurring near
    // an uncorroborated boundary is retained only as unresolved bounded evidence.
    const auto ungrounded_boundary = infer_cadential_degree_relation(
        key,
        g_major_100,
        c_major_200,
        arrival_between(g_major_100, c_major_200, false));
    CHECK(ungrounded_boundary.kind == cadential_degree_relation_kind::unresolved);
    CHECK(ungrounded_boundary.confidence <= unresolved_cadential_degree_relation_ceiling);
    CHECK(!ungrounded_boundary.phrase_grounded);

    // Root-motion ambiguity also blocks cadence-shaped degree naming, even when
    // the key-relative roots themselves can still be enumerated.
    const auto unreliable_root = infer_cadential_degree_relation(
        key,
        g_major_100,
        c_major_200,
        arrival_between(g_major_100, c_major_200, true, false));
    CHECK(unreliable_root.kind == cadential_degree_relation_kind::unresolved);
    CHECK(!unreliable_root.root_motion_grounded);
    CHECK(unreliable_root.confidence <= unresolved_cadential_degree_relation_ceiling);

    // A chromatic root cannot be squeezed into the diatonic cadence-shaped
    // vocabulary. B-flat -> C remains unresolved here.
    const auto b_flat_major_700 = triad(10, tertian_triad_quality::major, 700);
    const auto c_major_800 = triad(0, tertian_triad_quality::major, 800);
    const auto chromatic_arrival = infer_cadential_degree_relation(
        key,
        b_flat_major_700,
        c_major_800,
        arrival_between(b_flat_major_700, c_major_800));
    CHECK(chromatic_arrival.kind == cadential_degree_relation_kind::unresolved);
    CHECK(chromatic_arrival.confidence <= unresolved_cadential_degree_relation_ceiling);

    // Evidence from a different harmonic transition cannot be loaned into this
    // relation merely because its root interval and arrival time look plausible.
    bool transition_time_mismatch_rejected = false;
    try {
        const auto stale_g = triad(7, tertian_triad_quality::major, 50);
        const auto stale_arrival = arrival_between(stale_g, c_major_200);
        (void)infer_cadential_degree_relation(
            key,
            g_major_100,
            c_major_200,
            stale_arrival);
    } catch (const std::invalid_argument&) {
        transition_time_mismatch_rejected = true;
    }
    CHECK(transition_time_mismatch_rejected);

    // Endpoint quality is part of harmonic identity too. A G-minor chord at the
    // same root/time cannot borrow an arrival licensed by G-major -> C-major.
    bool transition_quality_mismatch_rejected = false;
    try {
        const auto g_minor_100 = triad(7, tertian_triad_quality::minor, 100);
        (void)infer_cadential_degree_relation(
            key,
            g_minor_100,
            c_major_200,
            five_to_one_arrival);
    } catch (const std::invalid_argument&) {
        transition_quality_mismatch_rejected = true;
    }
    CHECK(transition_quality_mismatch_rejected);

    // A mutated root-motion summary is inconsistent with the retained endpoint
    // identities and must fail closed.
    bool root_motion_mismatch_rejected = false;
    try {
        auto mismatched = five_to_one_arrival;
        mismatched.root_motion_semitones = 7;
        (void)infer_cadential_degree_relation(
            key,
            g_major_100,
            c_major_200,
            mismatched);
    } catch (const std::invalid_argument&) {
        root_motion_mismatch_rejected = true;
    }
    CHECK(root_motion_mismatch_rejected);

    // The supplied key hypothesis itself remains a live provenance contract.
    // A same-named key region that does not cover these chord observations is
    // not interchangeable with the one that actually licensed them.
    bool key_region_mismatch_rejected = false;
    try {
        auto wrong_region = key;
        wrong_region.region = time_span{at(300), at(1000)};
        (void)infer_cadential_degree_relation(
            wrong_region,
            g_major_100,
            c_major_200,
            five_to_one_arrival);
    } catch (const std::invalid_argument&) {
        key_region_mismatch_rejected = true;
    }
    CHECK(key_region_mismatch_rejected);

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
