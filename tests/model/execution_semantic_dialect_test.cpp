#include "model/execution_semantic_dialect.h"

#include <cassert>
#include <stdexcept>
#include <string>

using namespace vgmtooling::model;

namespace {
const std::string& string_attribute(const node& value, const std::string& key) {
    const auto* found = execution_string_attribute(value, key);
    assert(found != nullptr);
    return *found;
}
} // namespace

int main() {
    musical_execution_graph graph;

    execution_dialect_identity mds;
    mds.family = "MDSDRV ctrmml";
    mds.revision = "2022-05-21";
    mds.artifact_role = execution_artifact_role::authoring_source;
    mds.origin = execution_semantic_origin::external_document;
    mds.source = "ctrmml MML reference";
    mds.detail = "composer-facing MML lowers into MDSDRV sequence data";
    const node_id mds_id = append_execution_dialect_identity(graph, mds);

    execution_dialect_capability_observation mds_portamento;
    mds_portamento.dialect = mds_id;
    mds_portamento.layer = semantic_layer::authored_program;
    mds_portamento.capability = execution_semantic_capability::pitch_without_retrigger;
    mds_portamento.state = execution_capability_state::supported;
    mds_portamento.origin = execution_semantic_origin::source_native;
    mds_portamento.status = evidence_status::exact;
    mds_portamento.source = "MML source fixture";
    mds_portamento.native_token = "G";
    mds_portamento.detail = "authoring source explicitly requests portamento";
    const node_id mds_pitch_id = append_execution_dialect_capability(graph, mds_portamento);

    execution_dialect_identity echo;
    echo.family = "Echo ESF";
    echo.revision = "documented stream format";
    echo.artifact_role = execution_artifact_role::runtime_sequence;
    echo.origin = execution_semantic_origin::external_document;
    echo.source = "Echo ESF documentation";
    echo.detail = "runtime arrangement stream exposes raw pitch changes without retrigger";
    const node_id echo_id = append_execution_dialect_identity(graph, echo);

    execution_dialect_capability_observation echo_slide;
    echo_slide.dialect = echo_id;
    echo_slide.layer = semantic_layer::driver_execution;
    echo_slide.capability = execution_semantic_capability::pitch_without_retrigger;
    echo_slide.state = execution_capability_state::supported;
    echo_slide.origin = execution_semantic_origin::external_document;
    echo_slide.source = "Echo ESF documentation";
    echo_slide.native_token = "$30-$3A frequency event";
    echo_slide.detail = "driver sequence changes raw pitch without issuing a note attack";
    const node_id echo_pitch_id = append_execution_dialect_capability(graph, echo_slide);

    const node* mds_pitch = graph.find_node(mds_pitch_id);
    const node* echo_pitch = graph.find_node(echo_pitch_id);
    assert(mds_pitch != nullptr && echo_pitch != nullptr);
    assert(string_attribute(*mds_pitch, "dialect_capability") == "pitch_without_retrigger");
    assert(string_attribute(*echo_pitch, "dialect_capability") == "pitch_without_retrigger");
    assert(string_attribute(*mds_pitch, "native_token") != string_attribute(*echo_pitch, "native_token"));

    execution_dialect_identity transformed;
    transformed.family = "VGM-derived runtime format";
    transformed.revision = "unknown";
    transformed.artifact_role = execution_artifact_role::transformed_runtime_sequence;
    transformed.origin = execution_semantic_origin::external_document;
    transformed.source = "conversion contract";
    transformed.detail = "runtime-oriented conversion begins from flattened chip execution";
    const node_id transformed_id = append_execution_dialect_identity(graph, transformed);

    bool rejected_false_authored_source = false;
    try {
        execution_dialect_capability_observation false_authored = mds_portamento;
        false_authored.dialect = transformed_id;
        append_execution_dialect_capability(graph, false_authored);
    } catch (const std::invalid_argument&) {
        rejected_false_authored_source = true;
    }
    assert(rejected_false_authored_source);

    execution_dialect_identity documented_driver;
    documented_driver.family = "documented legacy driver";
    documented_driver.revision = "A";
    documented_driver.artifact_role = execution_artifact_role::runtime_sequence;
    documented_driver.origin = execution_semantic_origin::external_document;
    documented_driver.source = "driver manual";
    documented_driver.detail = "manual explicitly documents the dialect surface";
    const node_id documented_id = append_execution_dialect_identity(graph, documented_driver);

    execution_dialect_capability_observation unsupported;
    unsupported.dialect = documented_id;
    unsupported.layer = semantic_layer::driver_execution;
    unsupported.capability = execution_semantic_capability::pitch_without_retrigger;
    unsupported.state = execution_capability_state::unsupported;
    unsupported.origin = execution_semantic_origin::external_document;
    unsupported.source = "driver manual";
    unsupported.detail = "manual explicitly states this operation is not available in this revision";
    const node_id unsupported_id = append_execution_dialect_capability(graph, unsupported);

    execution_dialect_identity unidentified;
    unidentified.family = "unidentified legacy driver";
    unidentified.revision = "unknown";
    unidentified.artifact_role = execution_artifact_role::unknown;
    unidentified.origin = execution_semantic_origin::runtime_observed;
    unidentified.source = "runtime capture";
    unidentified.detail = "driver family is not yet reconstructed beyond a bounded identity placeholder";
    const node_id unidentified_id = append_execution_dialect_identity(graph, unidentified);

    execution_dialect_capability_observation unknown = unsupported;
    unknown.dialect = unidentified_id;
    unknown.state = execution_capability_state::unknown;
    unknown.origin = execution_semantic_origin::unavailable;
    unknown.source = "runtime capture";
    unknown.detail = "capture cannot establish whether the driver could express this operation";
    const node_id unknown_id = append_execution_dialect_capability(graph, unknown);

    const node* unsupported_node = graph.find_node(unsupported_id);
    const node* unknown_node = graph.find_node(unknown_id);
    assert(unsupported_node != nullptr && unknown_node != nullptr);
    assert(string_attribute(*unsupported_node, "capability_state") == "unsupported");
    assert(string_attribute(*unknown_node, "capability_state") == "unknown");
    assert(has_flag(unknown_node->provenance.front().flags, provenance_flag::incomplete));

    bool rejected_token_for_absent_capability = false;
    try {
        execution_dialect_capability_observation invalid = unsupported;
        invalid.native_token = "imaginary_command";
        append_execution_dialect_capability(graph, invalid);
    } catch (const std::invalid_argument&) {
        rejected_token_for_absent_capability = true;
    }
    assert(rejected_token_for_absent_capability);

    return 0;
}
