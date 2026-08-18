#include "SNES_SPC.h"

#include "components/spc/spc_native_routed_source_projection.h"
#include "components/spc/spc_realtime_musical_omniphony_pipeline.h"
#include "components/spc/spc_runtime_capture.h"
#include "components/spc/spc_runtime_instrumentation_sink.h"
#include "components/spc/spc_runtime_spatial_adapter.h"
#include "components/spc/spc_snapshot.h"
#include "components/spc/spc_snapshot_spatial_seed.h"
#include "components/spc/spc_source_bus.h"
#include "model/realtime_spatial_governor_trace_validation.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef RETRO_VGM_COMPILER_FORENSIC_COMMIT
#define RETRO_VGM_COMPILER_FORENSIC_COMMIT "unknown"
#endif

#ifndef SNES_SPC_FORENSIC_COMMIT
#define SNES_SPC_FORENSIC_COMMIT "unknown"
#endif

#ifndef SNES_SPC_FORENSIC_PATCH_CONTRACT
#define SNES_SPC_FORENSIC_PATCH_CONTRACT "unknown"
#endif

namespace {

using namespace gameaudio::spc;
using namespace vgmtooling::model;

constexpr std::uint64_t spc_clock_rate = 1024000;
constexpr std::uint64_t native_sample_rate = 32000;
constexpr std::uint64_t clocks_per_native_frame = spc_clock_rate / native_sample_rate;
constexpr std::size_t governor_window_frames = 2048;
constexpr std::size_t source_lane_count = 10;
constexpr std::size_t dry_voice_count = 8;
constexpr std::uint32_t echo_generation = 1;
constexpr std::uint32_t renderer_ramp_frames = 96;
constexpr float validator_tolerance = 1.0e-4f;

static_assert(spc_clock_rate % native_sample_rate == 0);
static_assert(clocks_per_native_frame == 32);
static_assert(sizeof(short) == sizeof(std::int16_t));

using adapter_type = spc_runtime_spatial_adapter<governor_window_frames, spc_runtime_capture::capacity>;
using projection_type = spc_native_routed_source_projection_storage<
    governor_window_frames,
    spc_runtime_capture::capacity>;
using pipeline_type = spc_realtime_musical_omniphony_pipeline<
    source_lane_count,
    spc_runtime_capture::capacity,
    128>;
using trace_type = typename pipeline_type::pipeline_type::governor_trace_type;

std::vector<std::uint8_t> read_binary(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("could not open SPC input");
    input.seekg(0, std::ios::end);
    const std::streamoff end = input.tellg();
    if (end < 0)
        throw std::runtime_error("could not determine SPC input size");
    input.seekg(0, std::ios::beg);
    const auto size = static_cast<std::uint64_t>(end);
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        throw std::runtime_error("SPC input is too large for this process");

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        if (!input)
            throw std::runtime_error("could not read complete SPC input");
    }
    return bytes;
}

std::uint64_t parse_seconds(const char* text)
{
    if (text == nullptr || *text == '\0')
        throw std::invalid_argument("seconds must be a positive integer");
    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (end == text || *end != '\0' || value == 0 || value > 600)
        throw std::invalid_argument("seconds must be an integer in [1, 600]");
    return static_cast<std::uint64_t>(value);
}

bool valid_sha256(const std::string& value)
{
    if (value.size() != 64)
        return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0 || (ch >= 'a' && ch <= 'f');
    });
}

class spatial_runtime_sink final : public spc_runtime_instrumentation_sink {
public:
    void reset() noexcept
    {
        capture_.reset_trace();
        ram_write_serial_ = 0;
        tick_rate_ = 0;
        last_tick_ = 0;
        have_last_tick_ = false;
        invalid_ = false;
    }

    void begin_window() noexcept
    {
        capture_.begin_window();
    }

    bool seed_snapshot(const spc_snapshot& snapshot) noexcept
    {
        const auto seed = make_spc_snapshot_spatial_seed(snapshot, spc_clock_rate);
        for (auto record : seed) {
            if (!observe_time(record.tick, record.tick_rate))
                return false;
            capture_.observe(record);
        }
        return !invalid_ && !capture_.overflowed();
    }

    spc_runtime_spatial_capture_view view() const noexcept
    {
        return {
            capture_.records(),
            capture_.count(),
            capture_.overflowed() || invalid_,
        };
    }

    bool valid() const noexcept
    {
        return !invalid_ && !capture_.overflowed();
    }

    std::uint64_t observe_apuram_write(
        spc_runtime_ram_write_origin,
        std::int64_t tick,
        std::uint64_t tick_rate,
        std::uint16_t,
        const std::uint8_t* bytes,
        std::size_t byte_count) override
    {
        if (bytes == nullptr || byte_count == 0 || !observe_time(tick, tick_rate)) {
            invalid_ = true;
            return ram_write_serial_;
        }
        if (ram_write_serial_ == std::numeric_limits<std::uint64_t>::max()) {
            invalid_ = true;
            return ram_write_serial_;
        }
        return ++ram_write_serial_;
    }

    void observe_voice_event(spc_runtime_capture_record record) override
    {
        if (!observe_time(record.tick, record.tick_rate)) {
            invalid_ = true;
            return;
        }
        record.ram_write_serial = ram_write_serial_;
        capture_.observe(record);
        if (capture_.overflowed())
            invalid_ = true;
    }

private:
    bool observe_time(std::int64_t tick, std::uint64_t tick_rate) noexcept
    {
        if (tick < 0 || tick_rate == 0)
            return false;
        if (tick_rate_ != 0 && tick_rate != tick_rate_)
            return false;
        if (have_last_tick_ && tick < last_tick_)
            return false;
        if (tick_rate_ == 0)
            tick_rate_ = tick_rate;
        last_tick_ = tick;
        have_last_tick_ = true;
        return true;
    }

    spc_runtime_capture capture_{};
    std::uint64_t ram_write_serial_ = 0;
    std::uint64_t tick_rate_ = 0;
    std::int64_t last_tick_ = 0;
    bool have_last_tick_ = false;
    bool invalid_ = false;
};

class native_spatial_window {
public:
    void begin_window() noexcept
    {
        count_ = 0;
        valid_ = true;
    }

    static void observe(
        void* user,
        short const* dry_voice,
        int dry_count,
        short echo_left,
        short echo_right)
    {
        auto* self = static_cast<native_spatial_window*>(user);
        if (self == nullptr)
            return;
        self->observe_frame(dry_voice, dry_count, echo_left, echo_right);
    }

    bool valid_for(std::size_t expected_frames) const noexcept
    {
        return valid_ && count_ == expected_frames;
    }

    const float* dry(std::size_t voice) const noexcept
    {
        return voice < dry_voice_count ? dry_[voice].data() : nullptr;
    }
    const float* echo_left() const noexcept { return echo_left_.data(); }
    const float* echo_right() const noexcept { return echo_right_.data(); }

private:
    void observe_frame(
        short const* dry_voice,
        int dry_count,
        short echo_left,
        short echo_right) noexcept
    {
        if (!valid_ || dry_voice == nullptr || dry_count != static_cast<int>(dry_voice_count) ||
            count_ >= governor_window_frames) {
            valid_ = false;
            return;
        }

        constexpr float scale = 1.0f / 32768.0f;
        for (std::size_t voice = 0; voice < dry_voice_count; ++voice)
            dry_[voice][count_] = static_cast<float>(dry_voice[voice]) * scale;
        echo_left_[count_] = static_cast<float>(echo_left) * scale;
        echo_right_[count_] = static_cast<float>(echo_right) * scale;
        ++count_;
    }

    std::array<std::array<float, governor_window_frames>, dry_voice_count> dry_{};
    std::array<float, governor_window_frames> echo_left_{};
    std::array<float, governor_window_frames> echo_right_{};
    std::size_t count_ = 0;
    bool valid_ = true;
};

struct fake_renderer_state {
    std::size_t budget_calls = 0;
    std::size_t process_calls = 0;
};

std::uint32_t fake_abi_major() { return 0; }
std::uint32_t fake_abi_minor() { return 4; }

std::int32_t fake_reset(void*)
{
    return 0;
}

std::int32_t fake_set_mix_budget(
    void* processor,
    const omniphony_source_mix_budget_v1_transport* budget)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    if (state == nullptr || budget == nullptr)
        return -10;
    ++state->budget_calls;
    return 0;
}

std::int32_t fake_process(
    void* processor,
    const float* input,
    const omniphony_source_evidence_v1_transport* sources,
    std::size_t source_count,
    const omniphony_source_evidence_event_v1_transport*,
    std::size_t,
    std::size_t frame_count,
    std::uint64_t,
    std::uint32_t,
    float* output)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    if (state == nullptr || input == nullptr || sources == nullptr || output == nullptr ||
        source_count != source_lane_count)
        return -11;
    ++state->process_calls;
    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        output[frame * 2u] = 0.0f;
        output[frame * 2u + 1u] = 0.0f;
    }
    return 0;
}

const char* lane_kind_name(spatial_audio_lane_kind kind) noexcept
{
    switch (kind) {
    case spatial_audio_lane_kind::dry_source:
        return "dry_source";
    case spatial_audio_lane_kind::shared_effect_return:
        return "shared_effect_return";
    case spatial_audio_lane_kind::reference_mix:
        return "reference_mix";
    }
    return "unknown";
}

const char* trace_error_name(realtime_spatial_governor_trace_error error) noexcept
{
    switch (error) {
    case realtime_spatial_governor_trace_error::none: return "none";
    case realtime_spatial_governor_trace_error::trace_not_valid: return "trace_not_valid";
    case realtime_spatial_governor_trace_error::invalid_sequence: return "invalid_sequence";
    case realtime_spatial_governor_trace_error::invalid_timing: return "invalid_timing";
    case realtime_spatial_governor_trace_error::invalid_threshold: return "invalid_threshold";
    case realtime_spatial_governor_trace_error::nonfinite_budget: return "nonfinite_budget";
    case realtime_spatial_governor_trace_error::renderer_budget_mismatch: return "renderer_budget_mismatch";
    case realtime_spatial_governor_trace_error::invalid_source_observation: return "invalid_source_observation";
    case realtime_spatial_governor_trace_error::scene_count_mismatch: return "scene_count_mismatch";
    case realtime_spatial_governor_trace_error::scene_energy_mismatch: return "scene_energy_mismatch";
    case realtime_spatial_governor_trace_error::pair_overlap_mismatch: return "pair_overlap_mismatch";
    case realtime_spatial_governor_trace_error::sequence_index_mismatch: return "sequence_index_mismatch";
    case realtime_spatial_governor_trace_error::sequence_sample_rate_mismatch: return "sequence_sample_rate_mismatch";
    case realtime_spatial_governor_trace_error::sequence_position_mismatch: return "sequence_position_mismatch";
    case realtime_spatial_governor_trace_error::sequence_budget_mismatch: return "sequence_budget_mismatch";
    }
    return "unknown";
}

void write_budget(std::ostream& out, const realtime_spatial_mix_budget& budget)
{
    out << '{'
        << "\"dry_width_scale\":" << budget.dry_width_scale << ','
        << "\"dry_diffuse_scale\":" << budget.dry_diffuse_scale << ','
        << "\"depth_scale\":" << budget.depth_scale << ','
        << "\"height_scale\":" << budget.height_scale << ','
        << "\"shared_wet_strength\":" << budget.shared_wet_strength << ','
        << "\"shared_wet_extent\":" << budget.shared_wet_extent << ','
        << "\"added_externalization_scale\":" << budget.added_externalization_scale
        << '}';
}

void write_renderer_budget(
    std::ostream& out,
    const omniphony_source_mix_budget_v1_transport& budget)
{
    out << '{'
        << "\"depth_scale\":" << budget.depth_scale << ','
        << "\"height_scale\":" << budget.height_scale << ','
        << "\"shared_wet_strength_scale\":" << budget.shared_wet_strength_scale << ','
        << "\"shared_wet_extent_scale\":" << budget.shared_wet_extent_scale << ','
        << "\"externalization_scale\":" << budget.externalization_scale
        << '}';
}

void write_trace(
    std::ostream& out,
    const std::string& fixture_sha256,
    const trace_type& trace,
    const realtime_spatial_governor_trace_validation& validation,
    const std::optional<realtime_spatial_governor_trace_continuity>& continuity)
{
    out << "    {\n";
    out << "      \"fixture_sha256\":\"" << fixture_sha256 << "\",\n";
    out << "      \"trace_sequence_index\":" << trace.sequence_index << ",\n";
    out << "      \"absolute_sample_position\":" << trace.absolute_sample_position << ",\n";
    out << "      \"frame_count\":" << trace.frame_count << ",\n";
    out << "      \"sample_rate\":" << trace.sample_rate << ",\n";
    out << "      \"lane_count\":" << trace.lane_count << ",\n";
    out << "      \"observed_lane_count\":" << trace.scene.observed_lane_count << ",\n";
    out << "      \"active_lane_count\":" << trace.scene.active_lane_count << ",\n";
    out << "      \"shared_effect_energy_share\":" << trace.scene.shared_effect_energy_share << ",\n";
    out << "      \"coarse_spectral_overlap_scalar\":" << trace.scene.coarse_spectral_overlap << ",\n";
    out << "      \"active_dry_pair_count\":" << validation.active_dry_pair_count << ",\n";
    out << "      \"sources\":[\n";
    for (std::size_t lane = 0; lane < trace.lane_count; ++lane) {
        const auto& source = trace.sources[lane];
        out << "        {\"lane_index\":" << lane
            << ",\"lane_kind\":\"" << lane_kind_name(source.lane_kind) << "\""
            << ",\"source_id\":" << source.source_id
            << ",\"generation\":" << source.generation
            << ",\"audio_observed\":" << (source.audio_observed ? "true" : "false")
            << ",\"activity\":" << source.activity
            << ",\"relative_energy\":" << source.relative_energy
            << ",\"coarse_band_energy_share\":["
            << source.coarse_band_energy_share[0] << ','
            << source.coarse_band_energy_share[1] << ','
            << source.coarse_band_energy_share[2] << "]}"
            << (lane + 1u == trace.lane_count ? "\n" : ",\n");
    }
    out << "      ],\n";
    out << "      \"pairs\":[\n";
    bool first_pair = true;
    for (std::size_t left = 0; left < trace.lane_count; ++left) {
        for (std::size_t right = left + 1u; right < trace.lane_count; ++right) {
            realtime_spatial_overlap_pair_observation pair{};
            if (!trace.pair(left, right, pair))
                continue;
            if (!first_pair)
                out << ",\n";
            out << "        {\"left_lane_index\":" << pair.left_lane_index
                << ",\"right_lane_index\":" << pair.right_lane_index
                << ",\"left_source_id\":" << pair.left_source_id
                << ",\"left_generation\":" << pair.left_generation
                << ",\"right_source_id\":" << pair.right_source_id
                << ",\"right_generation\":" << pair.right_generation
                << ",\"coarse_spectral_overlap\":" << pair.coarse_spectral_overlap
                << ",\"pair_energy_weight\":" << pair.pair_energy_weight << '}';
            first_pair = false;
        }
    }
    if (!first_pair)
        out << '\n';
    out << "      ],\n";
    out << "      \"applied_budget\":";
    write_budget(out, trace.applied_budget);
    out << ",\n      \"renderer_abi_budget\":";
    write_renderer_budget(out, trace.renderer_budget);
    out << ",\n      \"learned_budget\":";
    write_budget(out, trace.learned_budget);
    out << ",\n";
    out << "      \"validation\":{\"valid\":"
        << (validation.valid ? "true" : "false")
        << ",\"error\":\"" << trace_error_name(validation.error) << "\""
        << ",\"reconstructed_coarse_spectral_overlap\":"
        << validation.reconstructed_coarse_spectral_overlap
        << ",\"strongest_pair_overlap\":" << validation.strongest_pair_overlap << "},\n";
    out << "      \"continuity\":";
    if (!continuity.has_value()) {
        out << "null\n";
    } else {
        out << "{\"valid\":" << (continuity->valid ? "true" : "false")
            << ",\"error\":\"" << trace_error_name(continuity->error) << "\"}\n";
    }
    out << "    }";
}

} // namespace

int main(int argc, char** argv)
{
    try {
        if (argc < 4 || argc > 5) {
            std::cerr
                << "usage: spc_spatial_governor_trace <input.spc> <output.json> "
                   "<fixture_sha256> [seconds]\n";
            return 2;
        }

        const std::string fixture_sha256 = argv[3];
        if (!valid_sha256(fixture_sha256))
            throw std::invalid_argument("fixture_sha256 must be 64 lowercase hexadecimal characters");
        const std::uint64_t seconds = argc == 5 ? parse_seconds(argv[4]) : 24u;
        const auto bytes = read_binary(argv[1]);
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<long>::max()))
            throw std::runtime_error("SPC input exceeds snes_spc load size range");
        const auto snapshot = parse_spc_snapshot(bytes.data(), bytes.size());

        SNES_SPC emulator{};
        if (const char* error = emulator.init())
            throw std::runtime_error(std::string{"snes_spc init failed: "} + error);
        if (const char* error = emulator.load_spc(
                bytes.data(),
                static_cast<long>(bytes.size())))
            throw std::runtime_error(std::string{"snes_spc load failed: "} + error);

        spatial_runtime_sink runtime{};
        runtime.reset();
        runtime.begin_window();
        if (!runtime.seed_snapshot(snapshot))
            throw std::runtime_error("could not seed exact snapshot spatial state");
        emulator.set_runtime_instrumentation_sink(&runtime);

        native_spatial_window native{};
        emulator.set_native_spatial_observer(native_spatial_window::observe, &native);

        auto adapter = std::make_unique<adapter_type>();
        auto projection = std::make_unique<projection_type>();
        auto pipeline = std::make_unique<pipeline_type>();
        fake_renderer_state renderer{};
        if (!pipeline->bind_renderer(
                static_cast<void*>(&renderer),
                fake_abi_major,
                fake_abi_minor,
                fake_reset,
                fake_set_mix_budget,
                fake_process))
            throw std::runtime_error("could not bind measurement-only Omniphony ABI 0.4 renderer");
        if (!pipeline->reset())
            throw std::runtime_error("could not reset measurement-only Omniphony pipeline");

        std::array<float, governor_window_frames * source_lane_count> source_scratch{};
        std::array<float, governor_window_frames * 2u> stereo_scratch{};
        std::array<spatial_audio_lane_view, source_lane_count> lanes{};
        std::optional<trace_type> previous_trace{};

        std::ofstream out(argv[2], std::ios::binary | std::ios::trunc);
        if (!out)
            throw std::runtime_error("could not open governor trace output");
        out << std::setprecision(9);
        out << "{\n";
        out << "  \"schema_version\":1,\n";
        out << "  \"model\":\"creator-blind SPC realtime spatial governor trace\",\n";
        out << "  \"claim_boundary\":\"Source/device spatial evidence and causal governor diagnostics only; no creator, game, track, genre or catalog labels enter runtime decisions. The bound Omniphony ABI is a measurement-only transport sink, not listening validation.\",\n";
        out << "  \"fixture_sha256\":\"" << fixture_sha256 << "\",\n";
        out << "  \"provenance\":{\n";
        out << "    \"retro_vgm_compiler_commit\":\"" << RETRO_VGM_COMPILER_FORENSIC_COMMIT << "\",\n";
        out << "    \"snes_spc_repository\":\"blarggs-audio-libraries/snes_spc\",\n";
        out << "    \"snes_spc_commit\":\"" << SNES_SPC_FORENSIC_COMMIT << "\",\n";
        out << "    \"instrumentation_patch_contract\":\"" << SNES_SPC_FORENSIC_PATCH_CONTRACT << "\"\n";
        out << "  },\n";
        out << "  \"execution\":{\"seconds\":" << seconds
            << ",\"native_sample_rate\":" << native_sample_rate
            << ",\"window_frames\":" << governor_window_frames
            << ",\"source_bytes\":" << bytes.size() << "},\n";
        out << "  \"traces\":[\n";

        const std::uint64_t total_frames = seconds * native_sample_rate;
        std::uint64_t reference_frame = 0;
        std::size_t trace_count = 0;
        bool first_trace = true;
        while (reference_frame < total_frames) {
            const std::size_t frames = static_cast<std::size_t>(std::min<std::uint64_t>(
                total_frames - reference_frame,
                governor_window_frames));
            native.begin_window();

            if (reference_frame != 0)
                runtime.begin_window();

            const std::size_t scalar_samples = frames * 2u;
            if (scalar_samples > static_cast<std::size_t>(std::numeric_limits<int>::max()))
                throw std::runtime_error("playback window exceeds snes_spc scalar sample range");
            if (const char* error = emulator.play(static_cast<int>(scalar_samples), nullptr))
                throw std::runtime_error(std::string{"snes_spc execution failed: "} + error);
            if (!native.valid_for(frames))
                throw std::runtime_error("native spatial observer did not produce one frame per S-DSP sample");
            if (!runtime.valid())
                throw std::runtime_error("runtime spatial evidence capture became invalid");

            const std::uint64_t window_start_tick = reference_frame * clocks_per_native_frame;
            if (!adapter->build_window(
                    runtime.view(),
                    static_cast<std::int64_t>(window_start_tick),
                    spc_clock_rate,
                    native_sample_rate,
                    frames))
                throw std::runtime_error("runtime spatial adapter rejected an exact playback window");

            for (std::size_t segment_index = 0;
                 segment_index < adapter->segment_count();
                 ++segment_index) {
                const auto& segment = adapter->segments()[segment_index];
                std::array<const float*, dry_voice_count> dry{};
                for (std::size_t voice = 0; voice < dry_voice_count; ++voice)
                    dry[voice] = native.dry(voice) + segment.reference_frame_offset;
                if (!projection->build(segment.sources, dry))
                    throw std::runtime_error("native routed source projection rejected a segment");

                const auto& projected = projection->block();
                for (std::size_t voice = 0; voice < dry_voice_count; ++voice)
                    lanes[voice] = projected.lanes[voice];

                lanes[8] = {};
                lanes[8].kind = spatial_audio_lane_kind::shared_effect_return;
                lanes[8].mono_pcm = native.echo_left() + segment.reference_frame_offset;
                lanes[8].evidence = spc_source_bus::make_post_evol_echo_source(
                    spc_source_bus::echo_side::left,
                    echo_generation);

                lanes[9] = {};
                lanes[9].kind = spatial_audio_lane_kind::shared_effect_return;
                lanes[9].mono_pcm = native.echo_right() + segment.reference_frame_offset;
                lanes[9].evidence = spc_source_bus::make_post_evol_echo_source(
                    spc_source_bus::echo_side::right,
                    echo_generation);

                spatial_source_block_view block = projected;
                block.lanes = lanes.data();
                block.lane_count = lanes.size();

                spatial_source_host_chunk chunk{};
                chunk.sources = block;
                chunk.session_epoch = 1;
                chunk.reference_frame_start =
                    reference_frame + static_cast<std::uint64_t>(segment.reference_frame_offset);

                const auto result = pipeline->process_chunk(
                    chunk,
                    static_cast<double>(native_sample_rate),
                    source_scratch.data(),
                    source_scratch.size(),
                    stereo_scratch.data(),
                    stereo_scratch.size(),
                    renderer_ramp_frames);
                if (!result.source_chunk_valid || !result.omniphony.prepared ||
                    !result.omniphony.budget_committed || !result.omniphony.rendered ||
                    !result.omniphony.learned || result.omniphony.governor_trace_index == 0)
                    throw std::runtime_error("causal SPC Omniphony process did not produce an admitted trace");

                const trace_type trace = pipeline->pipeline().last_governor_trace();
                if (trace.sequence_index != result.omniphony.governor_trace_index)
                    throw std::runtime_error("governor trace transaction identity mismatch");
                const auto validation = validate_realtime_spatial_governor_trace(
                    trace,
                    validator_tolerance);
                if (!validation.valid)
                    throw std::runtime_error(
                        std::string{"governor trace validation failed: "}
                        + trace_error_name(validation.error));

                std::optional<realtime_spatial_governor_trace_continuity> continuity{};
                if (previous_trace.has_value()) {
                    continuity = validate_realtime_spatial_governor_trace_continuity(
                        *previous_trace,
                        trace,
                        validator_tolerance);
                    if (!continuity->valid)
                        throw std::runtime_error(
                            std::string{"governor trace continuity failed: "}
                            + trace_error_name(continuity->error));
                }

                if (!first_trace)
                    out << ",\n";
                write_trace(out, fixture_sha256, trace, validation, continuity);
                first_trace = false;
                ++trace_count;
                previous_trace = trace;
            }

            reference_frame += static_cast<std::uint64_t>(frames);
        }

        emulator.set_native_spatial_observer(nullptr, nullptr);
        emulator.set_runtime_instrumentation_sink(nullptr);
        if (trace_count == 0)
            throw std::runtime_error("controlled execution produced no governor traces");

        out << "\n  ],\n";
        out << "  \"summary\":{\"trace_count\":" << trace_count
            << ",\"renderer_budget_calls\":" << renderer.budget_calls
            << ",\"renderer_process_calls\":" << renderer.process_calls
            << ",\"all_traces_admitted\":true}\n";
        out << "}\n";
        if (!out)
            throw std::runtime_error("failed while writing governor trace output");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "spc_spatial_governor_trace: " << error.what() << '\n';
        return 1;
    }
}
