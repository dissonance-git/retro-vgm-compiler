#include "model/execution_semantic_provenance.h"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>

using namespace vgmtooling::model;

namespace {

node_id add_identical_synthesis_state(musical_execution_graph& graph) {
    node synthesis;
    synthesis.kind = node_kind::synthesis_object;
    synthesis.layer = semantic_layer::synthesis;
    synthesis.flow = flow_kind::value;
    synthesis.label = "identical YM2612 chip state";
    synthesis.attributes.push_back({
        "frequency_number",
        std::uint64_t{0x269},
        evidence_status::exact,
        1.0,
        "",
    });
    synthesis.attributes.push_back({
        "algorithm",
        std::uint64_t{4},
        evidence_status::exact,
        1.0,
        "",
    });
    return graph.add_node(std::move(synthesis));
}

const attribute* find_attribute(const node& value, const std::string& key) {
    for (const auto& attribute : value.attributes) {
        if (attribute.key == key)
            return &attribute;
    }
    return nullptr;
}

} // namespace

int main() {
    musical_execution_graph graph;

    execution_semantic_observation tied_note;
    tied_note.layer = semantic_layer::driver_execution;
    tied_note.kind = execution_semantic_kind::articulation_control;
    tied_note.origin = execution_semantic_origin::deterministic_reconstruction;
    tied_note.status = evidence_status::derived;
    tied_note.source = "driver-disassembly";
    tied_note.native_token = "do_not_attack_next_note=1";
    tied_note.detail = "driver state suppresses the next hardware retrigger";
    const node_id tied_note_id = append_execution_semantic_observation(graph, tied_note);

    execution_semantic_observation attacked_note = tied_note;
    attacked_note.native_token = "do_not_attack_next_note=0";
    attacked_note.detail = "driver state permits a fresh hardware attack";
    const node_id attacked_note_id = append_execution_semantic_observation(graph, attacked_note);

    const node_id tied_chip_state = add_identical_synthesis_state(graph);
    const node_id attacked_chip_state = add_identical_synthesis_state(graph);

    link_execution_semantic_ancestry(
        graph,
        tied_note_id,
        tied_chip_state,
        evidence_status::derived,
        1.0,
        "driver-disassembly",
        "same synthesis state arrived through retrigger suppression");
    link_execution_semantic_ancestry(
        graph,
        attacked_note_id,
        attacked_chip_state,
        evidence_status::derived,
        1.0,
        "driver-disassembly",
        "same synthesis state arrived through a fresh attack path");

    const auto tied_ancestors = direct_execution_semantic_ancestors(graph, tied_chip_state);
    const auto attacked_ancestors = direct_execution_semantic_ancestors(graph, attacked_chip_state);
    assert(tied_ancestors.size() == 1);
    assert(attacked_ancestors.size() == 1);
    assert(tied_ancestors.front()->id == tied_note_id);
    assert(attacked_ancestors.front()->id == attacked_note_id);
    assert(tied_ancestors.front()->id != attacked_ancestors.front()->id);

    // Documentary sources can justify an investigation without silently becoming
    // exact runtime state. The graph marks them as external annotations.
    execution_semantic_observation documentary;
    documentary.layer = semantic_layer::source_representation;
    documentary.kind = execution_semantic_kind::authored_command;
    documentary.origin = execution_semantic_origin::external_document;
    documentary.status = evidence_status::derived;
    documentary.confidence = 0.9;
    documentary.source = "composer-interview";
    documentary.detail = "composer-facing driver/tool context exists above chip writes";
    const node_id documentary_id = append_execution_semantic_observation(graph, documentary);
    const node* documentary_node = graph.find_node(documentary_id);
    assert(documentary_node != nullptr);
    assert(documentary_node->provenance.size() == 1);
    assert(has_flag(
        documentary_node->provenance.front().flags,
        provenance_flag::external_annotation));

    execution_semantic_gap missing_source;
    missing_source.layer = semantic_layer::authored_program;
    missing_source.kind = execution_semantic_gap_kind::intent_underdetermined;
    missing_source.source = "vgm-runtime-capture";
    missing_source.detail =
        "flattened register execution does not preserve the upstream articulation command";
    const node_id gap_id = append_execution_semantic_gap(graph, missing_source);
    const node* gap_node = graph.find_node(gap_id);
    assert(gap_node != nullptr);
    assert(has_flag(gap_node->provenance.front().flags, provenance_flag::incomplete));
    const attribute* available = find_attribute(*gap_node, "semantic_available");
    assert(available != nullptr);
    assert(std::get<bool>(available->value) == false);
    const attribute* gap_kind = find_attribute(*gap_node, "gap_kind");
    assert(gap_kind != nullptr);
    assert(std::get<std::string>(gap_kind->value) == "intent_underdetermined");

    const node_id unknown_chip_state = add_identical_synthesis_state(graph);
    link_execution_semantic_ancestry(
        graph,
        gap_id,
        unknown_chip_state,
        evidence_status::exact,
        1.0,
        "vgm-runtime-capture",
        "absence is preserved rather than replaced with a guessed upstream command");
    const auto unknown_ancestors = direct_execution_semantic_ancestors(graph, unknown_chip_state);
    assert(unknown_ancestors.size() == 1);
    assert(unknown_ancestors.front()->id == gap_id);

    bool rejected_hypothesis_status = false;
    try {
        execution_semantic_observation invalid = tied_note;
        invalid.origin = execution_semantic_origin::hypothesis;
        invalid.status = evidence_status::derived;
        append_execution_semantic_observation(graph, invalid);
    } catch (const std::invalid_argument&) {
        rejected_hypothesis_status = true;
    }
    assert(rejected_hypothesis_status);

    bool rejected_downward_ancestry = false;
    try {
        link_execution_semantic_ancestry(
            graph,
            tied_note_id,
            documentary_id,
            evidence_status::derived,
            1.0,
            "test",
            "driver evidence cannot be treated as an ancestor of source representation");
    } catch (const std::invalid_argument&) {
        rejected_downward_ancestry = true;
    }
    assert(rejected_downward_ancestry);

    return 0;
}
