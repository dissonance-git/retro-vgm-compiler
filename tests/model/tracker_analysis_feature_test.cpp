#include "model/analysis_feature.h"

#include <cassert>
#include <cstdint>
#include <string>

using namespace vgmtooling::model;

int main() {
    analysis_feature_set tracker;

    tracker.add(present_feature(
        "authored_note_index",
        semantic_layer::authored_program,
        attribute_value{std::uint64_t{61}},
        evidence_status::exact,
        1.0,
        "tracker_note_index"));
    tracker.add(present_feature(
        "logical_source_channel",
        semantic_layer::authored_program,
        attribute_value{std::uint64_t{3}},
        evidence_status::exact,
        1.0,
        "channel"));
    tracker.add(present_feature(
        "pattern_index",
        semantic_layer::authored_program,
        attribute_value{std::uint64_t{7}},
        evidence_status::exact,
        1.0,
        "pattern"));
    tracker.add(present_feature(
        "row_index",
        semantic_layer::authored_program,
        attribute_value{std::uint64_t{32}},
        evidence_status::exact,
        1.0,
        "row"));
    tracker.add(present_feature(
        "instrument_command",
        semantic_layer::authored_program,
        attribute_value{std::uint64_t{5}},
        evidence_status::exact,
        1.0,
        "instrument_index"));
    tracker.add(present_feature(
        "effect_command",
        semantic_layer::authored_program,
        attribute_value{std::string{"tone_portamento"}},
        evidence_status::exact,
        1.0));

    // The exact source cell can state a note target and a tone-portamento
    // command without proving the realized pitch at this instant. Execution
    // must apply prior channel state and format-specific tick/effect semantics.
    tracker.add(unresolved_feature(
        "performed_absolute_pitch",
        semantic_layer::musical_performance,
        feature_availability::unknown,
        "authored note target is exact, but tone-portamento execution state is required before performed pitch is known",
        "OpenMPT tracker semantic fixture"));
    tracker.add(unresolved_feature(
        "pitch_trajectory",
        semantic_layer::musical_performance,
        feature_availability::unknown,
        "exact portamento command does not by itself determine the realized tick-by-tick trajectory",
        "OpenMPT tracker semantic fixture"));

    // The logical source channel is stronger identity evidence than a hardware
    // register lane, but it is still not automatically a persistent musical part.
    tracker.add(unresolved_feature(
        "persistent_part_identity",
        semantic_layer::musical_performance,
        feature_availability::unknown,
        "tracker channel identity is authored source topology, not automatic persistent musical-part identity",
        "OpenMPT tracker semantic fixture"));

    // A static tracker cell does not expose one renderer's bounded physical
    // synthesis allocation or one listener's auditory organization.
    tracker.add(unresolved_feature(
        "physical_voice_episode_id",
        semantic_layer::synthesis,
        feature_availability::unavailable,
        "static pattern cell does not expose runtime synthesis allocation",
        "OpenMPT tracker semantic fixture"));
    tracker.add(unresolved_feature(
        "auditory_stream_identity",
        semantic_layer::auditory_interpretation,
        feature_availability::unavailable,
        "symbolic pattern cell alone does not establish listener-level stream grouping",
        "OpenMPT tracker semantic fixture"));

    const analysis_feature* note = tracker.find("authored_note_index");
    const analysis_feature* channel = tracker.find("logical_source_channel");
    const analysis_feature* effect = tracker.find("effect_command");
    const analysis_feature* performed_pitch = tracker.find("performed_absolute_pitch");
    const analysis_feature* trajectory = tracker.find("pitch_trajectory");
    const analysis_feature* part = tracker.find("persistent_part_identity");
    const analysis_feature* physical = tracker.find("physical_voice_episode_id");
    const analysis_feature* auditory = tracker.find("auditory_stream_identity");

    assert(note != nullptr);
    assert(channel != nullptr);
    assert(effect != nullptr);
    assert(performed_pitch != nullptr);
    assert(trajectory != nullptr);
    assert(part != nullptr);
    assert(physical != nullptr);
    assert(auditory != nullptr);

    assert(note->availability == feature_availability::present);
    assert(note->claim_layer == semantic_layer::authored_program);
    assert(note->status == evidence_status::exact);
    assert(channel->availability == feature_availability::present);
    assert(channel->claim_layer == semantic_layer::authored_program);
    assert(effect->availability == feature_availability::present);
    assert(std::get<std::string>(effect->value.value()) == "tone_portamento");

    assert(performed_pitch->availability == feature_availability::unknown);
    assert(performed_pitch->claim_layer == semantic_layer::musical_performance);
    assert(!performed_pitch->value.has_value());
    assert(trajectory->availability == feature_availability::unknown);
    assert(trajectory->claim_layer == semantic_layer::musical_performance);
    assert(part->availability == feature_availability::unknown);
    assert(part->claim_layer == semantic_layer::musical_performance);

    assert(physical->availability == feature_availability::unavailable);
    assert(physical->claim_layer == semantic_layer::synthesis);
    assert(auditory->availability == feature_availability::unavailable);
    assert(auditory->claim_layer == semantic_layer::auditory_interpretation);

    return 0;
}
