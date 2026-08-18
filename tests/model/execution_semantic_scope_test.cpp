#include "model/execution_semantic_scope.h"

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

    execution_dialect_identity cube;
    cube.family = "Cube command stream";
    cube.revision = "documented Mega Drive family";
    cube.artifact_role = execution_artifact_role::runtime_sequence;
    cube.origin = execution_semantic_origin::external_document;
    cube.source = "CubeDocs music.txt";
    cube.detail = "channel-class-specific command sets share opcode space";
    const node_id cube_id = append_execution_dialect_identity(graph, cube);

    scoped_execution_semantic_observation cube_fm_fa;
    cube_fm_fa.dialect = cube_id;
    cube_fm_fa.semantic.layer = semantic_layer::driver_execution;
    cube_fm_fa.semantic.kind = execution_semantic_kind::driver_command;
    cube_fm_fa.semantic.origin = execution_semantic_origin::external_document;
    cube_fm_fa.semantic.status = evidence_status::exact;
    cube_fm_fa.semantic.source = "CubeDocs music.txt";
    cube_fm_fa.semantic.native_token = "$FA";
    cube_fm_fa.semantic.detail = "FM command sets stereo panning";
    cube_fm_fa.scope.kind = execution_semantic_scope_kind::fm_channel;
    cube_fm_fa.scope.detail = "YM FM command set";
    cube_fm_fa.timing.kind = execution_timing_domain_kind::driver_update;
    const node_id cube_fm_fa_id = append_scoped_execution_semantic_observation(graph, cube_fm_fa);

    scoped_execution_semantic_observation cube_psg_fa;
    cube_psg_fa.dialect = cube_id;
    cube_psg_fa.semantic.layer = semantic_layer::driver_execution;
    cube_psg_fa.semantic.kind = execution_semantic_kind::timing_control;
    cube_psg_fa.semantic.origin = execution_semantic_origin::external_document;
    cube_psg_fa.semantic.status = evidence_status::exact;
    cube_psg_fa.semantic.source = "CubeDocs music.txt";
    cube_psg_fa.semantic.native_token = "$FA";
    cube_psg_fa.semantic.detail = "PSG tone command changes YM2612 Timer B update timing";
    cube_psg_fa.scope.kind = execution_semantic_scope_kind::psg_tone_channel;
    cube_psg_fa.scope.detail = "PSG tone command set";
    cube_psg_fa.timing.kind = execution_timing_domain_kind::ym_timer_b;
    cube_psg_fa.timing.detail = "Timer B defines sound update frequency";
    const node_id cube_psg_fa_id = append_scoped_execution_semantic_observation(graph, cube_psg_fa);

    const node* fm_fa = graph.find_node(cube_fm_fa_id);
    const node* psg_fa = graph.find_node(cube_psg_fa_id);
    assert(fm_fa != nullptr && psg_fa != nullptr);
    assert(string_attribute(*fm_fa, "native_token") == "$FA");
    assert(string_attribute(*psg_fa, "native_token") == "$FA");
    assert(string_attribute(*fm_fa, "semantic_scope") == "fm_channel");
    assert(string_attribute(*psg_fa, "semantic_scope") == "psg_tone_channel");
    assert(string_attribute(*fm_fa, "semantic_kind") != string_attribute(*psg_fa, "semantic_kind"));
    assert(string_attribute(*psg_fa, "timing_domain") == "ym_timer_b");

    execution_dialect_identity minimusic;
    minimusic.family = "MiniMusic";
    minimusic.revision = "1.19";
    minimusic.artifact_role = execution_artifact_role::runtime_sequence;
    minimusic.origin = execution_semantic_origin::external_document;
    minimusic.source = "MiniMusic format.md";
    minimusic.detail = "documented Z80-resident track bytecode";
    const node_id minimusic_id = append_execution_dialect_identity(graph, minimusic);

    scoped_execution_dialect_capability_observation mini_no_retrigger;
    mini_no_retrigger.capability.dialect = minimusic_id;
    mini_no_retrigger.capability.layer = semantic_layer::driver_execution;
    mini_no_retrigger.capability.capability = execution_semantic_capability::pitch_without_retrigger;
    mini_no_retrigger.capability.state = execution_capability_state::supported;
    mini_no_retrigger.capability.origin = execution_semantic_origin::external_document;
    mini_no_retrigger.capability.status = evidence_status::exact;
    mini_no_retrigger.capability.source = "MiniMusic format.md";
    mini_no_retrigger.capability.native_token = "$61 cancel next key";
    mini_no_retrigger.capability.detail = "next key-on changes pitch and waits without performing the key operation";
    mini_no_retrigger.scope.kind = execution_semantic_scope_kind::physical_channel;
    mini_no_retrigger.timing.kind = execution_timing_domain_kind::track_tempo_divider;
    const node_id mini_no_retrigger_id = append_scoped_execution_dialect_capability(graph, mini_no_retrigger);

    execution_dialect_identity flame;
    flame.family = "FlameDriver";
    flame.revision = "SonicDriverVer 5";
    flame.artifact_role = execution_artifact_role::runtime_sequence;
    flame.origin = execution_semantic_origin::external_document;
    flame.source = "FlameDriver source";
    flame.detail = "Sonic-family driver with explicit no-attack, slide, sustain-frequency and envelope state";
    const node_id flame_id = append_execution_dialect_identity(graph, flame);

    scoped_execution_dialect_capability_observation flame_no_retrigger;
    flame_no_retrigger.capability.dialect = flame_id;
    flame_no_retrigger.capability.layer = semantic_layer::driver_execution;
    flame_no_retrigger.capability.capability = execution_semantic_capability::pitch_without_retrigger;
    flame_no_retrigger.capability.state = execution_capability_state::supported;
    flame_no_retrigger.capability.origin = execution_semantic_origin::external_document;
    flame_no_retrigger.capability.status = evidence_status::exact;
    flame_no_retrigger.capability.source = "FlameDriver source";
    flame_no_retrigger.capability.native_token = "bitNoAttack";
    flame_no_retrigger.capability.detail = "note parser and key-on/off paths suppress retrigger while preserving track evolution";
    flame_no_retrigger.scope.kind = execution_semantic_scope_kind::logical_track;
    flame_no_retrigger.timing.kind = execution_timing_domain_kind::track_tempo_divider;
    const node_id flame_no_retrigger_id = append_scoped_execution_dialect_capability(graph, flame_no_retrigger);

    const node* mini = graph.find_node(mini_no_retrigger_id);
    const node* flame_node = graph.find_node(flame_no_retrigger_id);
    assert(mini != nullptr && flame_node != nullptr);
    assert(string_attribute(*mini, "dialect_capability") == "pitch_without_retrigger");
    assert(string_attribute(*flame_node, "dialect_capability") == "pitch_without_retrigger");
    assert(string_attribute(*mini, "native_token") != string_attribute(*flame_node, "native_token"));
    assert(string_attribute(*mini, "semantic_scope") == "physical_channel");
    assert(string_attribute(*flame_node, "semantic_scope") == "logical_track");

    execution_dialect_identity sonic2;
    sonic2.family = "Sonic 2 SMPS";
    sonic2.revision = "disassembled Z80 driver";
    sonic2.artifact_role = execution_artifact_role::runtime_sequence;
    sonic2.origin = execution_semantic_origin::external_document;
    sonic2.source = "Sonic 2 sound driver disassembly";
    sonic2.detail = "classic driver exposes a global tempo modifier plus per-track timing divisor";
    const node_id sonic2_id = append_execution_dialect_identity(graph, sonic2);

    scoped_execution_semantic_observation note_fill;
    note_fill.dialect = sonic2_id;
    note_fill.semantic.layer = semantic_layer::driver_execution;
    note_fill.semantic.kind = execution_semantic_kind::timing_control;
    note_fill.semantic.origin = execution_semantic_origin::external_document;
    note_fill.semantic.status = evidence_status::exact;
    note_fill.semantic.source = "Sonic 2 sound driver disassembly";
    note_fill.semantic.native_token = "NoteFillMaster";
    note_fill.semantic.detail = "note-fill cutoff is interpreted inside track timing state rather than as an absolute wall-clock duration";
    note_fill.scope.kind = execution_semantic_scope_kind::logical_track;
    note_fill.timing.kind = execution_timing_domain_kind::track_tempo_divider;
    note_fill.timing.detail = "track durations are multiplied by TempoDivider before countdown";
    const node_id note_fill_id = append_scoped_execution_semantic_observation(graph, note_fill);
    const node* note_fill_node = graph.find_node(note_fill_id);
    assert(note_fill_node != nullptr);
    assert(string_attribute(*note_fill_node, "timing_domain") == "track_tempo_divider");

    bool rejected_global_index = false;
    try {
        execution_semantic_scope invalid;
        invalid.kind = execution_semantic_scope_kind::global;
        invalid.index = 0;
        validate_execution_semantic_scope(invalid);
    } catch (const std::invalid_argument&) {
        rejected_global_index = true;
    }
    assert(rejected_global_index);

    bool rejected_unspecified_rate = false;
    try {
        execution_timing_domain invalid;
        invalid.kind = execution_timing_domain_kind::unspecified;
        invalid.nominal_hz = 60.0;
        validate_execution_timing_domain(invalid);
    } catch (const std::invalid_argument&) {
        rejected_unspecified_rate = true;
    }
    assert(rejected_unspecified_rate);

    return 0;
}
