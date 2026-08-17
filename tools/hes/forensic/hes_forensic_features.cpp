#include "hes_forensic_capture.h"

#include <gme/Hes_Emu.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef RETRO_VGM_COMPILER_FORENSIC_COMMIT
#define RETRO_VGM_COMPILER_FORENSIC_COMMIT "unknown"
#endif
#ifndef LIBGME_HES_FORENSIC_COMMIT
#define LIBGME_HES_FORENSIC_COMMIT "unknown"
#endif
#ifndef LIBGME_HES_FORENSIC_HOOK_CONTRACT
#define LIBGME_HES_FORENSIC_HOOK_CONTRACT "unknown"
#endif

namespace {

class forensic_hes_emu final : public Hes_Emu {
public:
    std::int64_t forensic_clock_rate() const {
        return static_cast<std::int64_t>(clock_rate());
    }

    blargg_err_t run_forensic_clocks(std::int32_t requested_clocks) {
        blip_time_t duration = requested_clocks;
        const blargg_err_t error = run_clocks(duration, 0);
        if (error == nullptr && duration != requested_clocks)
            return "HES forensic run changed the requested clock duration";
        return error;
    }
};

struct options {
    std::filesystem::path hes_path;
    std::filesystem::path m3u_path;
    bool has_m3u = false;
    int track_index = -1;
    double seconds = 60.0;
    std::filesystem::path output_path;
};

[[noreturn]] void usage_error(const std::string& detail) {
    throw std::invalid_argument(
        detail +
        "\nusage: hes_forensic_features <input.hes> --track N "
        "[--m3u playlist.m3u] [--seconds S] --json output.json");
}

options parse_options(int argc, char** argv) {
    if (argc < 2)
        usage_error("missing HES input");

    options result;
    result.hes_path = argv[1];
    for (int index = 2; index < argc; ++index) {
        const std::string argument = argv[index];
        auto require_value = [&](const char* name) -> std::string {
            if (++index >= argc)
                usage_error(std::string("missing value for ") + name);
            return argv[index];
        };

        if (argument == "--track") {
            result.track_index = std::stoi(require_value("--track"));
        } else if (argument == "--m3u") {
            result.m3u_path = require_value("--m3u");
            result.has_m3u = true;
        } else if (argument == "--seconds") {
            result.seconds = std::stod(require_value("--seconds"));
        } else if (argument == "--json") {
            result.output_path = require_value("--json");
        } else {
            usage_error("unknown argument: " + argument);
        }
    }

    if (result.track_index < 0)
        usage_error("--track must be a nonnegative playlist/raw track index");
    if (!std::isfinite(result.seconds) || result.seconds <= 0.0 || result.seconds > 600.0)
        usage_error("--seconds must be finite and in (0, 600]");
    if (result.output_path.empty())
        usage_error("--json is required");
    return result;
}

void require_ok(blargg_err_t error, const char* operation) {
    if (error != nullptr)
        throw std::runtime_error(std::string(operation) + ": " + error);
}

int consume_warning(forensic_hes_emu& emu) {
    return emu.warning() == nullptr ? 0 : 1;
}

void write_integer_array(
    std::ostream& out,
    const std::vector<vgmtooling::hes::apu_write_observation>& writes,
    int field) {
    out << '[';
    for (std::size_t index = 0; index < writes.size(); ++index) {
        if (index != 0)
            out << ',';
        if (field == 0)
            out << writes[index].clock;
        else if (field == 1)
            out << static_cast<unsigned>(writes[index].register_offset);
        else
            out << static_cast<unsigned>(writes[index].data);
    }
    out << ']';
}

void write_adpcm_integer_array(
    std::ostream& out,
    const std::vector<vgmtooling::hes::adpcm_write_observation>& writes,
    int field) {
    out << '[';
    for (std::size_t index = 0; index < writes.size(); ++index) {
        if (index != 0)
            out << ',';
        if (field == 0)
            out << writes[index].clock;
        else if (field == 1)
            out << writes[index].register_offset;
        else
            out << static_cast<unsigned>(writes[index].data);
    }
    out << ']';
}

void write_json(
    const options& opts,
    std::int64_t clock_rate,
    std::int64_t captured_clocks,
    std::uintmax_t source_size,
    std::uintmax_t playlist_size,
    int warning_count,
    const std::vector<vgmtooling::hes::apu_write_observation>& apu_writes,
    const std::vector<vgmtooling::hes::adpcm_write_observation>& adpcm_writes) {
    opts.output_path.parent_path().empty()
        ? void()
        : std::filesystem::create_directories(opts.output_path.parent_path());
    std::ofstream out(opts.output_path, std::ios::binary);
    if (!out)
        throw std::runtime_error("could not open JSON output");

    out << "{\n";
    out << "  \"model\": \"creator-blind HES forensic register sidecar\",\n";
    out << "  \"schema_version\": 1,\n";
    out << "  \"claim_boundary\": \"Ordered HuC6280 PSG and PC Engine CD ADPCM writes only. Physical channel selection is an execution observation, not persistent musical identity.\",\n";
    out << "  \"provenance\": {\n";
    out << "    \"retro_vgm_compiler_commit\": \"" << RETRO_VGM_COMPILER_FORENSIC_COMMIT << "\",\n";
    out << "    \"libgme_repository\": \"https://github.com/libgme/game-music-emu\",\n";
    out << "    \"libgme_commit\": \"" << LIBGME_HES_FORENSIC_COMMIT << "\",\n";
    out << "    \"instrumentation_contract\": \"" << LIBGME_HES_FORENSIC_HOOK_CONTRACT << "\",\n";
    out << "    \"clock_rate_hz\": " << clock_rate << "\n";
    out << "  },\n";
    out << "  \"capture\": {\n";
    out << "    \"track_index\": " << opts.track_index << ",\n";
    out << "    \"playlist_loaded\": " << (opts.has_m3u ? "true" : "false") << ",\n";
    out << "    \"source_size_bytes\": " << source_size << ",\n";
    out << "    \"playlist_size_bytes\": " << playlist_size << ",\n";
    out << "    \"requested_seconds\": " << opts.seconds << ",\n";
    out << "    \"captured_clocks\": " << captured_clocks << ",\n";
    out << "    \"warning_count\": " << warning_count << ",\n";
    out << "    \"capture_complete\": " << (warning_count == 0 ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"psg_writes\": {\n";
    out << "    \"count\": " << apu_writes.size() << ",\n";
    out << "    \"clock\": ";
    write_integer_array(out, apu_writes, 0);
    out << ",\n    \"register_offset\": ";
    write_integer_array(out, apu_writes, 1);
    out << ",\n    \"data\": ";
    write_integer_array(out, apu_writes, 2);
    out << "\n  },\n";
    out << "  \"adpcm_writes\": {\n";
    out << "    \"count\": " << adpcm_writes.size() << ",\n";
    out << "    \"clock\": ";
    write_adpcm_integer_array(out, adpcm_writes, 0);
    out << ",\n    \"register_offset\": ";
    write_adpcm_integer_array(out, adpcm_writes, 1);
    out << ",\n    \"data\": ";
    write_adpcm_integer_array(out, adpcm_writes, 2);
    out << "\n  }\n";
    out << "}\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        const options opts = parse_options(argc, argv);
        if (!std::filesystem::is_regular_file(opts.hes_path))
            throw std::runtime_error("HES input is not a regular file");
        if (opts.has_m3u && !std::filesystem::is_regular_file(opts.m3u_path))
            throw std::runtime_error("M3U input is not a regular file");

        forensic_hes_emu emu;
        require_ok(emu.set_sample_rate(44100), "set_sample_rate");
        require_ok(emu.load_file(opts.hes_path.string().c_str()), "load HES");
        int warning_count = consume_warning(emu);
        if (opts.has_m3u) {
            require_ok(emu.load_m3u(opts.m3u_path.string().c_str()), "load M3U");
            warning_count += consume_warning(emu);
        }
        if (opts.track_index >= emu.track_count())
            throw std::runtime_error("track index is outside the loaded HES/M3U track set");
        require_ok(emu.start_track(opts.track_index), "start HES track");
        warning_count += consume_warning(emu);

        // We need execution state, not PCM. Null all six normal HES output lanes
        // so long forensic runs do not accumulate audio-buffer work.
        emu.mute_voices(-1);

        const std::int64_t clock_rate = emu.forensic_clock_rate();
        if (clock_rate <= 0)
            throw std::runtime_error("HES clock rate is unavailable");
        const long double requested =
            static_cast<long double>(opts.seconds) * static_cast<long double>(clock_rate);
        if (requested > static_cast<long double>(std::numeric_limits<std::int64_t>::max()))
            throw std::runtime_error("requested HES duration is too large");
        const std::int64_t target_clocks = static_cast<std::int64_t>(std::llround(requested));
        const std::int64_t nominal_chunk = std::max<std::int64_t>(1, clock_rate / 20);

        std::vector<vgmtooling::hes::apu_write_observation> apu_writes;
        std::vector<vgmtooling::hes::adpcm_write_observation> adpcm_writes;
        vgmtooling::hes::scoped_apu_write_capture apu_capture(apu_writes);
        vgmtooling::hes::scoped_adpcm_write_capture adpcm_capture(adpcm_writes);

        std::int64_t epoch = 0;
        while (epoch < target_clocks) {
            const std::int64_t remaining = target_clocks - epoch;
            const std::int64_t chunk64 = std::min(nominal_chunk, remaining);
            if (chunk64 > std::numeric_limits<std::int32_t>::max())
                throw std::runtime_error("HES forensic chunk exceeds blip_time_t range");
            const auto chunk = static_cast<std::int32_t>(chunk64);
            vgmtooling::hes::set_apu_write_epoch(epoch);
            vgmtooling::hes::set_adpcm_write_epoch(epoch);
            require_ok(emu.run_forensic_clocks(chunk), "run HES clocks");
            warning_count += consume_warning(emu);
            epoch += chunk;
        }

        write_json(
            opts,
            clock_rate,
            epoch,
            std::filesystem::file_size(opts.hes_path),
            opts.has_m3u ? std::filesystem::file_size(opts.m3u_path) : 0,
            warning_count,
            apu_writes,
            adpcm_writes);
        return warning_count == 0 ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "hes_forensic_features: " << error.what() << '\n';
        return 1;
    }
}
