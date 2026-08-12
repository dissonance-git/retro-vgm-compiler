#include "../../components/spc/spc_runtime_sample_adapter.h"
#include "../../components/spc/spc_runtime_voice_adapter.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static const attribute* find_attribute(const node* value, const char* key) {
    if (value == nullptr)
        return nullptr;
    for (const auto& item : value->attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

static const attribute* find_attribute(const edge* value, const char* key) {
    if (value == nullptr)
        return nullptr;
    for (const auto& item : value->attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

static std::array<std::uint8_t, spc_full_file_size> make_spc_fixture() {
    std::array<std::uint8_t, spc_full_file_size> bytes{};
    static constexpr char signature[] = "SNES-SPC700 Sound File Data v0.30\x1A\x1A";
    static_assert(sizeof(signature) - 1 == spc_signature_size, "unexpected SPC signature size");
    std::memcpy(bytes.data(), signature, spc_signature_size);
    bytes[0x24] = 30;
    return bytes;
}

static void write_two_block_sample(
    std::array<std::uint8_t, spc_runtime_ram_size>& ram,
    std::uint16_t start,
    std::uint8_t payload_seed) {
    ram[start] = 0x00;
    for (std::uint16_t i = 1; i < 9; ++i)
        ram[static_cast<std::uint16_t>(start + i)] = static_cast<std::uint8_t>(payload_seed + i);
    const std::uint16_t second = static_cast<std::uint16_t>(start + 9u);
    ram[second] = 0x03; // END + LOOP.
    for (std::uint16_t i = 1; i < 9; ++i)
        ram[static_cast<std::uint16_t>(second + i)] = static_cast<std::uint8_t>(payload_seed + 0x20u + i);
}

static spc_voice_runtime_event runtime_event(
    spc_voice_runtime_event_kind kind,
    std::uint8_t voice,
    std::int64_t tick,
    std::uint8_t source,
    std::uint16_t start) {
    spc_voice_runtime_event event;
    event.kind = kind;
    event.voice = voice;
    event.tick = tick;
    event.tick_rate = 1024000;
    event.source_index = source;
    event.brr_address = start;
    return event;
}

static spc_runtime_sample_observation sample_observation(
    const spc_runtime_append_result& runtime,
    std::uint8_t source,
    std::uint16_t start,
    std::uint16_t loop,
    std::uint64_t ram_serial,
    std::int64_t tick) {
    spc_runtime_sample_observation observation;
    observation.trace_event_id = runtime.trace_event_id;
    observation.physical_voice_episode_id = runtime.physical_voice_episode_id;
    observation.source_index = source;
    observation.start_address = start;
    observation.directory_loop_address = loop;
    observation.ram_write_serial = ram_serial;
    observation.tick = tick;
    observation.tick_rate = 1024000;
    return observation;
}

int main() {
    const auto snapshot = parse_spc_snapshot(make_spc_fixture());

    musical_execution_graph graph;
    const auto snapshot_graph = materialize_spc_snapshot(
        graph,
        snapshot,
        "runtime-sample-fixture.spc",
        to_flags(provenance_flag::runtime_capture));
    auto runtime = begin_spc_runtime_voice_trace(
        graph,
        snapshot_graph,
        "runtime-sample-fixture.spc",
        to_flags(provenance_flag::none));
    auto samples = begin_spc_runtime_sample_graph(
        "runtime-sample-fixture.spc",
        to_flags(provenance_flag::none));

    std::array<std::uint8_t, spc_runtime_ram_size> live_ram{};
    write_two_block_sample(live_ram, 0x3000, 0x10);

    spc_ram_generation_tracker tracker;
    spc_ram_shadow shadow;
    CHECK(shadow.synchronize(live_ram.data(), tracker) == spc_ram_page_count);
    CHECK(shadow.synchronized_write_serial() == 0);

    auto key_on_event = runtime_event(
        spc_voice_runtime_event_kind::key_on_accepted,
        0,
        100,
        3,
        0x3000);
    key_on_event.key_on_delay = 19;
    const auto key_on = append_spc_runtime_voice_event(graph, runtime, key_on_event);
    CHECK(key_on.physical_voice_episode_id.has_value());

    auto phase_event = runtime_event(
        spc_voice_runtime_event_kind::sample_phase_started,
        0,
        119,
        3,
        0x3000);
    phase_event.key_on_delay = 0;
    const auto phase = append_spc_runtime_voice_event(graph, runtime, phase_event);
    CHECK(phase.physical_voice_episode_id.has_value());

    const auto first_observation = sample_observation(
        phase,
        3,
        0x3000,
        0x3009,
        tracker.write_serial(),
        119);
    const auto first = materialize_spc_runtime_sample(
        graph,
        samples,
        shadow,
        first_observation);

    CHECK(first.status == spc_runtime_sample_status::materialized);
    CHECK(first.sample_id.has_value());
    CHECK(first.reference_edge_id.has_value());
    CHECK(samples.cache.size() == 1);
    CHECK(graph.nodes_of_kind(node_kind::sample_buffer).size() == 1);

    const node_id first_sample_id = *first.sample_id;
    const node* first_sample = graph.find_node(first_sample_id);
    CHECK(first_sample != nullptr);
    CHECK(first_sample->kind == node_kind::sample_buffer);
    CHECK(first_sample->layer == semantic_layer::synthesis);
    CHECK(first_sample->flow == flow_kind::value);
    CHECK(!first_sample->active.has_value());
    CHECK(std::get<std::string>(find_attribute(first_sample, "identity_scope")->value) == "runtime_ram_version");
    CHECK(std::get<std::uint64_t>(find_attribute(first_sample, "start_address")->value) == 0x3000);
    CHECK(std::get<std::uint64_t>(find_attribute(first_sample, "block_count")->value) == 2);
    CHECK(std::get<std::uint64_t>(find_attribute(first_sample, "compressed_byte_count")->value) == 18);
    CHECK(std::get<bool>(find_attribute(first_sample, "event_time_ram_exact")->value));
    CHECK(std::get<std::string>(find_attribute(first_sample, "persistent_instrument_identity")->value) == "unresolved");

    const edge* first_reference = graph.find_edge(*first.reference_edge_id);
    CHECK(first_reference != nullptr);
    CHECK(first_reference->kind == edge_kind::references);
    CHECK(first_reference->from == phase.trace_event_id);
    CHECK(first_reference->to == first_sample_id);
    CHECK(!first_reference->active.has_value());
    CHECK(std::get<std::uint64_t>(find_attribute(first_reference, "source_index")->value) == 3);
    CHECK(std::get<std::uint64_t>(find_attribute(first_reference, "directory_loop_address")->value) == 0x3009);

    // Same stored bytes, same start address, no relevant writes. A later runtime
    // source observation reuses the exact RAM version even if reference-specific
    // loop metadata differs.
    const auto source_change = append_spc_runtime_voice_event(
        graph,
        runtime,
        runtime_event(
            spc_voice_runtime_event_kind::source_latched,
            0,
            140,
            4,
            0x3000));
    CHECK(source_change.physical_voice_episode_id.has_value());
    const auto second = materialize_spc_runtime_sample(
        graph,
        samples,
        shadow,
        sample_observation(
            source_change,
            4,
            0x3000,
            0x3000,
            tracker.write_serial(),
            140));
    CHECK(second.status == spc_runtime_sample_status::reused);
    CHECK(second.sample_id.has_value());
    CHECK(*second.sample_id == first_sample_id);
    CHECK(graph.nodes_of_kind(node_kind::sample_buffer).size() == 1);
    const edge* second_reference = graph.find_edge(*second.reference_edge_id);
    CHECK(std::get<std::uint64_t>(find_attribute(second_reference, "source_index")->value) == 4);
    CHECK(std::get<std::uint64_t>(find_attribute(second_reference, "directory_loop_address")->value) == 0x3000);

    // An unrelated APURAM mutation after an older source event does not destroy
    // exactness for the BRR pages observed by that event.
    const auto old_source = append_spc_runtime_voice_event(
        graph,
        runtime,
        runtime_event(
            spc_voice_runtime_event_kind::source_latched,
            0,
            160,
            4,
            0x3000));
    const std::uint64_t old_event_serial = tracker.write_serial();
    live_ram[0x5000] = 0xA5;
    tracker.mark_write(0x5000, 1);
    CHECK(shadow.synchronize(live_ram.data(), tracker) == 1);
    const auto after_unrelated_write = materialize_spc_runtime_sample(
        graph,
        samples,
        shadow,
        sample_observation(
            old_source,
            4,
            0x3000,
            0x3000,
            old_event_serial,
            160));
    CHECK(after_unrelated_write.status == spc_runtime_sample_status::reused);
    CHECK(*after_unrelated_write.sample_id == first_sample_id);

    // A relevant BRR-page write after the source event makes the later shadow an
    // invalid witness for that old event-time sample. No sample/reference is
    // invented from the changed memory.
    const auto before_mutation_event = append_spc_runtime_voice_event(
        graph,
        runtime,
        runtime_event(
            spc_voice_runtime_event_kind::source_latched,
            0,
            180,
            4,
            0x3000));
    const std::uint64_t before_mutation_serial = tracker.write_serial();
    live_ram[0x3001] ^= 0x7F;
    tracker.mark_write(0x3001, 1);
    CHECK(shadow.synchronize(live_ram.data(), tracker) == 1);
    const std::size_t references_before_reject = graph.edges_from(
        before_mutation_event.trace_event_id,
        edge_kind::references).size();
    const auto rejected_old_version = materialize_spc_runtime_sample(
        graph,
        samples,
        shadow,
        sample_observation(
            before_mutation_event,
            4,
            0x3000,
            0x3000,
            before_mutation_serial,
            180));
    CHECK(rejected_old_version.status == spc_runtime_sample_status::ram_changed_after_observation);
    CHECK(!rejected_old_version.sample_id.has_value());
    CHECK(!rejected_old_version.reference_edge_id.has_value());
    CHECK(graph.edges_from(before_mutation_event.trace_event_id, edge_kind::references).size() == references_before_reject);

    // Once a later event observes the already-mutated RAM, the same address can
    // legitimately materialize as a new RAM version. Address equality does not
    // force identity across mutation.
    const auto after_mutation_event = append_spc_runtime_voice_event(
        graph,
        runtime,
        runtime_event(
            spc_voice_runtime_event_kind::source_latched,
            0,
            200,
            4,
            0x3000));
    const auto new_version = materialize_spc_runtime_sample(
        graph,
        samples,
        shadow,
        sample_observation(
            after_mutation_event,
            4,
            0x3000,
            0x3000,
            tracker.write_serial(),
            200));
    CHECK(new_version.status == spc_runtime_sample_status::materialized);
    CHECK(new_version.sample_id.has_value());
    CHECK(*new_version.sample_id != first_sample_id);
    CHECK(graph.nodes_of_kind(node_kind::sample_buffer).size() == 2);
    CHECK(samples.cache.size() == 2);

    // If a source event is newer than the analysis shadow, exact reconstruction
    // waits instead of guessing from stale RAM.
    live_ram[0x6000] = 0x01;
    tracker.mark_write(0x6000, 1);
    const auto ahead_of_shadow_event = append_spc_runtime_voice_event(
        graph,
        runtime,
        runtime_event(
            spc_voice_runtime_event_kind::source_latched,
            0,
            220,
            4,
            0x3000));
    const auto shadow_behind = materialize_spc_runtime_sample(
        graph,
        samples,
        shadow,
        sample_observation(
            ahead_of_shadow_event,
            4,
            0x3000,
            0x3000,
            tracker.write_serial(),
            220));
    CHECK(shadow_behind.status == spc_runtime_sample_status::shadow_not_synchronized);
    CHECK(!shadow_behind.sample_id.has_value());

    // Synchronizing an unrelated newer page catches the shadow up. Because the
    // BRR page itself did not change after this event, the current RAM version is
    // again an exact witness and can be reused.
    CHECK(shadow.synchronize(live_ram.data(), tracker) == 1);
    const auto caught_up = materialize_spc_runtime_sample(
        graph,
        samples,
        shadow,
        sample_observation(
            ahead_of_shadow_event,
            4,
            0x3000,
            0x3000,
            tracker.write_serial(),
            220));
    CHECK(caught_up.status == spc_runtime_sample_status::reused);
    CHECK(caught_up.sample_id.has_value());
    CHECK(*caught_up.sample_id == *new_version.sample_id);

    bool rejected_unknown_trace = false;
    try {
        spc_runtime_sample_observation invalid;
        invalid.trace_event_id = 999999;
        invalid.start_address = 0x3000;
        invalid.ram_write_serial = tracker.write_serial();
        (void)materialize_spc_runtime_sample(graph, samples, shadow, invalid);
    } catch (const std::invalid_argument&) {
        rejected_unknown_trace = true;
    }
    CHECK(rejected_unknown_trace);

    // Runtime sample versions are still synthesis evidence only.
    CHECK(graph.nodes_of_kind(node_kind::instrument_definition).empty());
    CHECK(graph.nodes_of_kind(node_kind::musical_event).empty());
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::projection).empty());

    return 0;
}
