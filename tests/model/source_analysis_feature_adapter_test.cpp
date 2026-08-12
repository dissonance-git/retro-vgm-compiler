#include "components/vgm/enhancement/genesis_analysis_features.h"
#include "components/spc/spc_analysis_features.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

using namespace gameaudio::vgm;
using namespace gameaudio::spc;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

std::array<std::uint8_t, spc_full_file_size> make_spc_fixture() {
    std::array<std::uint8_t, spc_full_file_size> bytes{};
    static constexpr char signature[] = "SNES-SPC700 Sound File Data v0.30\x1A\x1A";
    static_assert(sizeof(signature) - 1 == spc_signature_size, "unexpected SPC signature size");
    std::memcpy(bytes.data(), signature, spc_signature_size);
    bytes[0x24] = 30;

    bytes[spc_dsp_offset + 0x04] = 0x03;
    bytes[spc_dsp_offset + 0x5D] = 0x20;
    const std::size_t directory_entry = spc_ram_offset + 0x200C;
    bytes[directory_entry + 0] = 0x00;
    bytes[directory_entry + 1] = 0x30;
    bytes[directory_entry + 2] = 0x00;
    bytes[directory_entry + 3] = 0x30;
    bytes[spc_ram_offset + 0x3000] = 0x01;
    return bytes;
}

const analysis_feature* require_feature(
    const analysis_feature_set& features,
    const char* name) {
    return features.find(name);
}

} // namespace

int main() {
    // Genesis path: use the actual VGM -> device shadow -> conservative
    // performance adapter, then extract analysis-facing feature states.
    musical_execution_graph genesis_graph;
    auto genesis = begin_genesis_performance_trace(
        genesis_graph,
        "feature-fixture.vgm",
        to_flags(provenance_flag::runtime_capture));

    const std::uint8_t fnum_high[] = {0xA4, 0x2C};
    const std::uint8_t fnum_low[] = {0xA0, 0x34};
    append_genesis_performance_event(
        genesis_graph,
        genesis,
        command_event{command_event_kind::command, 10, 0x100, 0x52, fnum_high, 2});
    append_genesis_performance_event(
        genesis_graph,
        genesis,
        command_event{command_event_kind::command, 10, 0x103, 0x52, fnum_low, 2});

    const std::uint8_t full_key_on[] = {0x28, 0xF0};
    const auto ym_on = append_genesis_performance_event(
        genesis_graph,
        genesis,
        command_event{command_event_kind::command, 20, 0x106, 0x52, full_key_on, 2});
    CHECK(ym_on.performance_event_id.has_value());
    CHECK(ym_on.physical_voice_episode_id.has_value());

    const auto ym_features = extract_genesis_performance_analysis_features(
        genesis_graph,
        *ym_on.performance_event_id);

    CHECK(require_feature(ym_features, "event_kind") != nullptr);
    CHECK(std::get<std::string>(require_feature(ym_features, "event_kind")->value.value()) ==
          "pitched_activity_onset");
    CHECK(std::get<std::string>(require_feature(ym_features, "device_family")->value.value()) ==
          "YM2612");
    CHECK(std::get<std::uint64_t>(require_feature(ym_features, "device_native_pitch_code")->value.value()) ==
          0x434);
    CHECK(std::get<std::uint64_t>(require_feature(ym_features, "device_native_pitch_block")->value.value()) ==
          5);
    CHECK(require_feature(ym_features, "device_native_pitch_code")->status == evidence_status::derived);
    CHECK(require_feature(ym_features, "physical_voice_episode_id")->availability ==
          feature_availability::present);
    CHECK(std::get<std::uint64_t>(require_feature(ym_features, "physical_voice_episode_id")->value.value()) ==
          *ym_on.physical_voice_episode_id);
    CHECK(require_feature(ym_features, "persistent_part_identity")->availability ==
          feature_availability::unknown);
    CHECK(require_feature(ym_features, "performed_pitch_frequency_hz")->availability ==
          feature_availability::unknown);
    CHECK(require_feature(ym_features, "performed_pitch_frequency_hz")->claim_layer ==
          semantic_layer::musical_performance);
    CHECK(require_feature(ym_features, "original_driver_track")->availability ==
          feature_availability::unavailable);
    CHECK(require_feature(ym_features, "sample_identity")->availability ==
          feature_availability::not_applicable);

    // A PSG tone uses a different native pitch representation. The feature
    // view keeps the common question while marking YM-specific block semantics
    // not applicable rather than zero.
    const std::uint8_t psg_tone_latch = 0x85;
    const std::uint8_t psg_tone_data = 0x12;
    append_genesis_performance_event(
        genesis_graph,
        genesis,
        command_event{command_event_kind::command, 70, 0x130, 0x50, &psg_tone_latch, 1});
    append_genesis_performance_event(
        genesis_graph,
        genesis,
        command_event{command_event_kind::command, 70, 0x132, 0x50, &psg_tone_data, 1});
    const std::uint8_t psg_unmute = 0x90;
    const auto psg_on = append_genesis_performance_event(
        genesis_graph,
        genesis,
        command_event{command_event_kind::command, 71, 0x134, 0x50, &psg_unmute, 1});
    CHECK(psg_on.performance_event_id.has_value());

    const auto psg_features = extract_genesis_performance_analysis_features(
        genesis_graph,
        *psg_on.performance_event_id);
    CHECK(std::get<std::string>(require_feature(psg_features, "device_family")->value.value()) ==
          "SN76489");
    CHECK(require_feature(psg_features, "device_native_pitch_code")->availability ==
          feature_availability::present);
    CHECK(require_feature(psg_features, "device_native_pitch_block")->availability ==
          feature_availability::not_applicable);
    CHECK(!require_feature(psg_features, "device_native_pitch_block")->value.has_value());

    // SPC path: begin from a real executable snapshot graph and controlled
    // S-DSP runtime observation. The extractor preserves exact device facts but
    // refuses to promote pitch rate/source index into performed frequency or part truth.
    const auto spc_bytes = make_spc_fixture();
    const auto snapshot = parse_spc_snapshot(spc_bytes);

    musical_execution_graph spc_graph;
    const auto snapshot_graph = materialize_spc_snapshot(
        spc_graph,
        snapshot,
        "feature-fixture.spc",
        to_flags(provenance_flag::runtime_capture));
    auto runtime = begin_spc_runtime_voice_trace(
        spc_graph,
        snapshot_graph,
        "feature-fixture.spc",
        to_flags(provenance_flag::none));

    spc_voice_runtime_event key_on;
    key_on.kind = spc_voice_runtime_event_kind::key_on_accepted;
    key_on.voice = 0;
    key_on.tick = 100;
    key_on.tick_rate = 1024000;
    key_on.source_index = 3;
    key_on.brr_address = 0x3000;
    key_on.envelope_value = 0;
    key_on.pitch_rate = 0x12340;
    key_on.key_on_delay = 19;
    key_on.noise_enabled = false;
    const auto spc_on = append_spc_runtime_voice_event(spc_graph, runtime, key_on);
    CHECK(spc_on.physical_voice_episode_id.has_value());

    const auto spc_features = extract_spc_runtime_analysis_features(
        spc_graph,
        spc_on.trace_event_id);
    CHECK(std::get<std::string>(require_feature(spc_features, "event_kind")->value.value()) ==
          "key_on_accepted");
    CHECK(require_feature(spc_features, "physical_voice")->status == evidence_status::exact);
    CHECK(std::get<std::uint64_t>(require_feature(spc_features, "physical_voice")->value.value()) == 0);
    CHECK(std::get<std::uint64_t>(require_feature(spc_features, "source_index")->value.value()) == 3);
    CHECK(std::get<std::uint64_t>(require_feature(spc_features, "device_native_pitch_rate")->value.value()) ==
          0x12340);
    CHECK(require_feature(spc_features, "physical_voice_episode_id")->availability ==
          feature_availability::present);
    CHECK(std::get<std::uint64_t>(require_feature(spc_features, "physical_voice_episode_id")->value.value()) ==
          *spc_on.physical_voice_episode_id);
    CHECK(require_feature(spc_features, "runtime_sample_version_id")->availability ==
          feature_availability::unknown);
    CHECK(require_feature(spc_features, "sample_root_tuning")->availability ==
          feature_availability::unknown);
    CHECK(require_feature(spc_features, "performed_pitch_frequency_hz")->availability ==
          feature_availability::unknown);
    CHECK(require_feature(spc_features, "performed_pitch_frequency_hz")->claim_layer ==
          semantic_layer::musical_performance);
    CHECK(require_feature(spc_features, "persistent_part_identity")->availability ==
          feature_availability::unknown);
    CHECK(require_feature(spc_features, "original_driver_track")->availability ==
          feature_availability::unavailable);

    // A global continuity-loss event is not a voice with zero-valued fields.
    // Per-voice questions become explicitly not applicable at that event.
    spc_voice_runtime_event gap;
    gap.kind = spc_voice_runtime_event_kind::continuation_lost;
    gap.tick = 120;
    gap.tick_rate = 1024000;
    const auto lost = append_spc_runtime_voice_event(spc_graph, runtime, gap);
    const auto gap_features = extract_spc_runtime_analysis_features(
        spc_graph,
        lost.trace_event_id);
    CHECK(require_feature(gap_features, "physical_voice")->availability ==
          feature_availability::not_applicable);
    CHECK(require_feature(gap_features, "device_native_pitch_rate")->availability ==
          feature_availability::not_applicable);
    CHECK(require_feature(gap_features, "physical_voice_episode_id")->availability ==
          feature_availability::not_applicable);
    CHECK(require_feature(gap_features, "performed_pitch_frequency_hz")->availability ==
          feature_availability::not_applicable);
    CHECK(require_feature(gap_features, "persistent_part_identity")->availability ==
          feature_availability::not_applicable);
    CHECK(require_feature(gap_features, "original_driver_track")->availability ==
          feature_availability::unavailable);

    return 0;
}
