#include "../../components/spc/spc_runtime_capture_adapter.h"

#include <array>
#include <cstdint>
#include <cstring>
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

static std::array<std::uint8_t, spc_full_file_size> make_spc_fixture() {
    std::array<std::uint8_t, spc_full_file_size> bytes{};
    static constexpr char signature[] = "SNES-SPC700 Sound File Data v0.30\x1A\x1A";
    static_assert(sizeof(signature) - 1 == spc_signature_size, "unexpected SPC signature size");
    std::memcpy(bytes.data(), signature, spc_signature_size);
    bytes[0x24] = 30;
    return bytes;
}

static void write_sample(
    std::array<std::uint8_t, spc_runtime_ram_size>& ram,
    std::uint16_t start) {
    ram[start] = 0x00;
    for (std::uint16_t i = 1; i < 9; ++i)
        ram[static_cast<std::uint16_t>(start + i)] = static_cast<std::uint8_t>(0x10 + i);
    const std::uint16_t second = static_cast<std::uint16_t>(start + 9u);
    ram[second] = 0x03;
    for (std::uint16_t i = 1; i < 9; ++i)
        ram[static_cast<std::uint16_t>(second + i)] = static_cast<std::uint8_t>(0x30 + i);
}

static spc_runtime_capture_record record(
    spc_voice_runtime_event_kind kind,
    std::uint64_t trace_index,
    std::int64_t tick,
    std::uint8_t voice,
    std::uint8_t source,
    std::uint16_t start,
    std::uint64_t ram_serial) {
    spc_runtime_capture_record value;
    value.kind = kind;
    value.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::source_index |
        spc_runtime_capture_field::brr_address |
        spc_runtime_capture_field::directory_loop_address |
        spc_runtime_capture_field::key_on_delay;
    value.trace_index = trace_index;
    value.tick = tick;
    value.tick_rate = 1024000;
    value.ram_write_serial = ram_serial;
    value.voice = voice;
    value.source_index = source;
    value.brr_address = start;
    value.directory_loop_address = static_cast<std::uint16_t>(start + 9u);
    value.key_on_delay = kind == spc_voice_runtime_event_kind::sample_phase_started ? 0 : 19;
    return value;
}

int main() {
    const auto snapshot = parse_spc_snapshot(make_spc_fixture());

    musical_execution_graph graph;
    const auto snapshot_graph = materialize_spc_snapshot(
        graph,
        snapshot,
        "capture-adapter-fixture.spc",
        to_flags(provenance_flag::runtime_capture));
    auto runtime = begin_spc_runtime_voice_trace(
        graph,
        snapshot_graph,
        "capture-adapter-fixture.spc",
        to_flags(provenance_flag::none));
    auto sample_graph = begin_spc_runtime_sample_graph(
        "capture-adapter-fixture.spc",
        to_flags(provenance_flag::none));
    spc_runtime_capture_materializer_state materializer;

    std::array<std::uint8_t, spc_runtime_ram_size> live_ram{};
    write_sample(live_ram, 0x3000);
    spc_ram_generation_tracker ram_tracker;
    spc_ram_shadow shadow;
    CHECK(shadow.synchronize(live_ram.data(), ram_tracker) == spc_ram_page_count);

    std::array<spc_runtime_capture_record, 2> first_window{
        record(
            spc_voice_runtime_event_kind::key_on_accepted,
            0,
            100,
            0,
            3,
            0x3000,
            ram_tracker.write_serial()),
        record(
            spc_voice_runtime_event_kind::sample_phase_started,
            1,
            119,
            0,
            3,
            0x3000,
            ram_tracker.write_serial()),
    };

    const auto first = materialize_spc_runtime_capture_window(
        graph,
        runtime,
        materializer,
        {first_window.data(), first_window.size(), false, 0, nullptr, 2},
        {&sample_graph, &shadow});

    CHECK(first.records_materialized == 2);
    CHECK(first.continuity_breaks == 0);
    CHECK(first.samples_materialized == 1);
    CHECK(first.samples_reused == 0);
    CHECK(first.samples_deferred == 0);
    CHECK(materializer.expected_trace_index.has_value());
    CHECK(*materializer.expected_trace_index == 2);
    CHECK(runtime.physical_voice_episode[0].has_value());
    const node_id first_episode_id = *runtime.physical_voice_episode[0];
    CHECK(graph.nodes_of_kind(node_kind::voice_instance).size() == 1);
    CHECK(graph.nodes_of_kind(node_kind::sample_buffer).size() == 1);

    const auto trace_events_after_first = graph.nodes_of_kind(node_kind::trace_event);
    CHECK(trace_events_after_first.size() == 2);
    CHECK(std::get<std::uint64_t>(find_attribute(trace_events_after_first[0], "trace_index")->value) == 0);
    CHECK(std::get<std::uint64_t>(find_attribute(trace_events_after_first[1], "trace_index")->value) == 1);
    CHECK(std::get<std::uint64_t>(find_attribute(trace_events_after_first[1], "ram_write_serial")->value) == 0);
    CHECK(std::get<std::string>(find_attribute(trace_events_after_first[1], "runtime_sample_materialization")->value) == "materialized");
    CHECK(graph.edges_from(trace_events_after_first[1]->id, edge_kind::references).size() == 1);

    // A normal next window continues ordinal and reuses the same exact RAM
    // version while keeping reference-specific source metadata on the new event.
    std::array<spc_runtime_capture_record, 1> second_window{
        record(
            spc_voice_runtime_event_kind::source_latched,
            2,
            140,
            0,
            4,
            0x3000,
            ram_tracker.write_serial()),
    };
    second_window[0].directory_loop_address = 0x3000;

    const auto second = materialize_spc_runtime_capture_window(
        graph,
        runtime,
        materializer,
        {second_window.data(), second_window.size(), false, 0, nullptr, 3},
        {&sample_graph, &shadow});
    CHECK(second.records_materialized == 1);
    CHECK(second.continuity_breaks == 0);
    CHECK(second.samples_reused == 1);
    CHECK(*materializer.expected_trace_index == 3);
    CHECK(*runtime.physical_voice_episode[0] == first_episode_id);
    CHECK(graph.nodes_of_kind(node_kind::sample_buffer).size() == 1);

    // Deliberately skip trace ordinal 3. The drain inserts an incomplete
    // continuity break before materializing ordinal 4. That closes the active
    // physical episode without fabricating a release or inactive event.
    std::array<spc_runtime_capture_record, 1> discontinuous_window{
        record(
            spc_voice_runtime_event_kind::source_latched,
            4,
            160,
            0,
            5,
            0x3000,
            ram_tracker.write_serial()),
    };

    const auto discontinuous = materialize_spc_runtime_capture_window(
        graph,
        runtime,
        materializer,
        {discontinuous_window.data(), discontinuous_window.size(), false, 0, nullptr, 5},
        {&sample_graph, &shadow});
    CHECK(discontinuous.records_materialized == 1);
    CHECK(discontinuous.continuity_breaks == 1);
    CHECK(!runtime.physical_voice_episode[0].has_value());
    CHECK(graph.find_node(first_episode_id)->active->end.has_value());
    CHECK(graph.find_node(first_episode_id)->active->end->tick == 160);
    CHECK(std::get<std::string>(find_attribute(graph.find_node(first_episode_id), "termination_reason")->value) == "semantic_continuation_lost");
    CHECK(!std::get<bool>(find_attribute(graph.find_node(first_episode_id), "termination_boundary_complete")->value));
    CHECK(*materializer.expected_trace_index == 5);

    const auto trace_events_after_gap = graph.nodes_of_kind(node_kind::trace_event);
    CHECK(trace_events_after_gap.size() == 5);
    const node* ordinal_gap = trace_events_after_gap[3];
    CHECK(std::get<std::string>(find_attribute(ordinal_gap, "event_kind")->value) == "continuation_lost");
    CHECK(std::get<std::uint64_t>(find_attribute(ordinal_gap, "trace_index")->value) == 3);
    CHECK(std::get<std::string>(find_attribute(ordinal_gap, "continuity_break_reason")->value) == "trace_index_discontinuity");
    CHECK(std::get<std::uint64_t>(find_attribute(ordinal_gap, "dropped_records")->value) == 1);
    CHECK(has_flag(ordinal_gap->provenance[0].flags, provenance_flag::incomplete));

    // Start a new episode at the expected ordinal, then emulate a capture
    // overflow whose first missing observation is exactly recorded.
    std::array<spc_runtime_capture_record, 1> before_overflow{
        record(
            spc_voice_runtime_event_kind::key_on_accepted,
            5,
            200,
            1,
            6,
            0x3000,
            ram_tracker.write_serial()),
    };
    spc_runtime_capture_record first_dropped = record(
        spc_voice_runtime_event_kind::source_latched,
        6,
        210,
        1,
        7,
        0x3000,
        ram_tracker.write_serial());

    const auto overflowed = materialize_spc_runtime_capture_window(
        graph,
        runtime,
        materializer,
        {before_overflow.data(), before_overflow.size(), true, 2, &first_dropped, 8},
        {&sample_graph, &shadow});
    CHECK(overflowed.records_materialized == 1);
    CHECK(overflowed.continuity_breaks == 1);
    CHECK(*materializer.expected_trace_index == 8);
    CHECK(!runtime.physical_voice_episode[1].has_value());

    const auto all_trace_events = graph.nodes_of_kind(node_kind::trace_event);
    const node* overflow_gap = all_trace_events.back();
    CHECK(std::get<std::string>(find_attribute(overflow_gap, "event_kind")->value) == "continuation_lost");
    CHECK(std::get<std::uint64_t>(find_attribute(overflow_gap, "trace_index")->value) == 6);
    CHECK(std::get<std::string>(find_attribute(overflow_gap, "continuity_break_reason")->value) == "capture_overflow");
    CHECK(std::get<std::uint64_t>(find_attribute(overflow_gap, "dropped_records")->value) == 2);
    CHECK(has_flag(overflow_gap->provenance[0].flags, provenance_flag::incomplete));

    // The next window may resume observation at the post-gap ordinal, but no
    // physical episode is silently bridged across the missing records.
    std::array<spc_runtime_capture_record, 1> after_overflow{
        record(
            spc_voice_runtime_event_kind::source_latched,
            8,
            230,
            1,
            7,
            0x3000,
            ram_tracker.write_serial()),
    };
    const auto resumed = materialize_spc_runtime_capture_window(
        graph,
        runtime,
        materializer,
        {after_overflow.data(), after_overflow.size(), false, 0, nullptr, 9},
        {&sample_graph, &shadow});
    CHECK(resumed.continuity_breaks == 0);
    CHECK(!runtime.physical_voice_episode[1].has_value());
    CHECK(*materializer.expected_trace_index == 9);

    CHECK(graph.nodes_of_kind(node_kind::musical_event).empty());
    CHECK(graph.nodes_of_kind(node_kind::part).empty());
    CHECK(graph.nodes_of_kind(node_kind::instrument_definition).empty());
    CHECK(graph.nodes_of_kind(node_kind::projection).empty());

    return 0;
}
