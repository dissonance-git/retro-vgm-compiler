extern "C" {
#include "emu/EmuStructs.h"
#include "emu/cores/nukedopm.h"
#include "emu/cores/ym2151.h"
}

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t kYm2151Clock = 3579545u;
constexpr std::size_t kFrames = 8192u;
constexpr std::size_t kTransientFrames = 2048u;
constexpr std::size_t kSteadyFrames = 2048u;
constexpr int kMaxCorrelationLag = 64;

using stereo_buffer = std::array<std::array<DEV_SMPL, kFrames>, 2>;

template <typename Function>
Function decode_function_pointer(void* raw) {
    static_assert(sizeof(Function) == sizeof(raw),
                  "libvgm function-pointer bridge assumes equal pointer widths");
    Function result{};
    std::memcpy(&result, &raw, sizeof(result));
    return result;
}

DEVFUNC_WRITE_A8D8 find_port_writer(const DEV_DEF& definition) {
    for (const DEVDEF_RWFUNC* rw = definition.rwFuncs;
         rw != nullptr && rw->funcPtr != nullptr;
         ++rw) {
        if (rw->funcType == (RWF_REGISTER | RWF_WRITE) && rw->rwType == DEVRW_A8D8)
            return decode_function_pointer<DEVFUNC_WRITE_A8D8>(rw->funcPtr);
    }
    return nullptr;
}

class opm_instance {
public:
    explicit opm_instance(const DEV_DEF& definition)
        : definition_(definition), write_(find_port_writer(definition)) {
        if (write_ == nullptr)
            throw std::runtime_error("YM2151 core exposes no production port writer");

        DEV_GEN_CFG config{};
        config.emuCore = definition.coreID;
        config.srMode = DEVRI_SRMODE_NATIVE;
        config.flags = 0;
        config.clock = kYm2151Clock;
        config.smplRate = 0;

        if (definition_.Start(&config, &info_) != 0u || info_.dataPtr == nullptr)
            throw std::runtime_error("YM2151 core failed to start");
        chip_ = info_.dataPtr->chipInf;
        if (chip_ == nullptr)
            throw std::runtime_error("YM2151 core returned no chip state");
        if (info_.sampleRate != kYm2151Clock / 64u)
            throw std::runtime_error("YM2151 cores did not enter the same native-rate domain");
        reset();
    }

    opm_instance(const opm_instance&) = delete;
    opm_instance& operator=(const opm_instance&) = delete;

    ~opm_instance() {
        if (chip_ != nullptr)
            definition_.Stop(chip_);
    }

    void reset() {
        definition_.Reset(chip_);
        if (definition_.SetMuteMask != nullptr)
            definition_.SetMuteMask(chip_, 0u);
    }

    // This exactly mirrors VGMPlayer::SendYMCommand for command 0x54:
    // ordinary RWF_WRITE, address port first, data port second.
    void write_register(std::uint8_t reg, std::uint8_t value) {
        write_(chip_, 0u, reg);
        write_(chip_, 1u, value);
    }

    stereo_buffer render() {
        stereo_buffer output{};
        DEV_SMPL* planes[2] = {output[0].data(), output[1].data()};
        definition_.Update(chip_, static_cast<UINT32>(kFrames), planes);
        return output;
    }

private:
    const DEV_DEF& definition_;
    DEV_INFO info_{};
    void* chip_ = nullptr;
    DEVFUNC_WRITE_A8D8 write_ = nullptr;
};

enum class fixture_id : std::uint8_t {
    algorithm7_attack,
    feedback_chain,
    lfo_modulation,
    channel8_noise,
};

constexpr std::array<fixture_id, 4> kFixtures{
    fixture_id::algorithm7_attack,
    fixture_id::feedback_chain,
    fixture_id::lfo_modulation,
    fixture_id::channel8_noise,
};

std::string_view fixture_name(fixture_id id) {
    switch (id) {
    case fixture_id::algorithm7_attack:
        return "algorithm7_attack";
    case fixture_id::feedback_chain:
        return "feedback_chain";
    case fixture_id::lfo_modulation:
        return "lfo_modulation";
    case fixture_id::channel8_noise:
        return "channel8_noise";
    }
    return "unknown";
}

void write_operator(
    opm_instance& chip,
    std::size_t channel,
    std::size_t physical_slot,
    std::uint8_t dt1_mul,
    std::uint8_t tl,
    std::uint8_t ks_ar,
    std::uint8_t am_d1r,
    std::uint8_t dt2_d2r,
    std::uint8_t d1l_rr) {
    const auto slot = static_cast<std::uint8_t>(channel + physical_slot * 8u);
    chip.write_register(static_cast<std::uint8_t>(0x40u + slot), dt1_mul);
    chip.write_register(static_cast<std::uint8_t>(0x60u + slot), tl);
    chip.write_register(static_cast<std::uint8_t>(0x80u + slot), ks_ar);
    chip.write_register(static_cast<std::uint8_t>(0xA0u + slot), am_d1r);
    chip.write_register(static_cast<std::uint8_t>(0xC0u + slot), dt2_d2r);
    chip.write_register(static_cast<std::uint8_t>(0xE0u + slot), d1l_rr);
}

void key_all_operators(opm_instance& chip, std::size_t channel) {
    chip.write_register(
        0x08u,
        static_cast<std::uint8_t>(0x78u | static_cast<std::uint8_t>(channel)));
}

void program_fixture(opm_instance& chip, fixture_id fixture) {
    switch (fixture) {
    case fixture_id::algorithm7_attack: {
        constexpr std::size_t channel = 0u;
        chip.write_register(0x20u, 0xC7u); // both outputs, algorithm 7
        chip.write_register(0x28u, 0x4Au);
        chip.write_register(0x30u, 0x00u);
        chip.write_register(0x38u, 0x00u);
        for (std::size_t slot = 0; slot < 4u; ++slot)
            write_operator(chip, channel, slot, 0x01u, 0x10u, 0x1Fu, 0x08u, 0x08u, 0x4Fu);
        key_all_operators(chip, channel);
        break;
    }
    case fixture_id::feedback_chain: {
        constexpr std::size_t channel = 2u;
        chip.write_register(0x22u, 0xF8u); // both outputs, feedback 7, algorithm 0
        chip.write_register(0x2Au, 0x52u);
        chip.write_register(0x32u, 0x80u);
        chip.write_register(0x3Au, 0x00u);
        write_operator(chip, channel, 0u, 0x02u, 0x08u, 0x1Fu, 0x06u, 0x04u, 0x3Fu);
        write_operator(chip, channel, 1u, 0x03u, 0x14u, 0x1Fu, 0x08u, 0x06u, 0x4Fu);
        write_operator(chip, channel, 2u, 0x01u, 0x18u, 0x1Fu, 0x0Au, 0x08u, 0x5Fu);
        write_operator(chip, channel, 3u, 0x01u, 0x00u, 0x1Fu, 0x0Cu, 0x08u, 0x5Fu);
        key_all_operators(chip, channel);
        break;
    }
    case fixture_id::lfo_modulation: {
        constexpr std::size_t channel = 4u;
        chip.write_register(0x18u, 0xE7u); // LFO frequency
        chip.write_register(0x19u, 0x5Fu); // AMD
        chip.write_register(0x19u, 0xDFu); // PMD
        chip.write_register(0x1Bu, 0x02u); // triangle LFO
        chip.write_register(0x24u, 0xC7u); // both outputs, algorithm 7
        chip.write_register(0x2Cu, 0x46u);
        chip.write_register(0x34u, 0x40u);
        chip.write_register(0x3Cu, 0x73u); // PMS 7, AMS 3
        for (std::size_t slot = 0; slot < 4u; ++slot)
            write_operator(chip, channel, slot, 0x01u, 0x18u, 0x1Fu, 0x9Fu, 0x0Cu, 0x6Fu);
        key_all_operators(chip, channel);
        break;
    }
    case fixture_id::channel8_noise: {
        constexpr std::size_t channel = 7u;
        chip.write_register(0x0Fu, 0x9Fu); // channel-8 noise enabled, period 31
        chip.write_register(0x27u, 0xC7u); // both outputs, algorithm 7
        chip.write_register(0x2Fu, 0x42u);
        chip.write_register(0x37u, 0x00u);
        chip.write_register(0x3Fu, 0x00u);
        for (std::size_t slot = 0; slot < 4u; ++slot)
            write_operator(chip, channel, slot, 0x01u, 0x20u, 0x1Fu, 0x08u, 0x08u, 0x4Fu);
        key_all_operators(chip, channel);
        break;
    }
    }
}

stereo_buffer run_fixture(opm_instance& chip, fixture_id fixture) {
    chip.reset();
    program_fixture(chip, fixture);
    return chip.render();
}

bool buffers_equal(const stereo_buffer& a, const stereo_buffer& b) {
    return a == b;
}

std::size_t first_audible_frame(const stereo_buffer& buffer) {
    for (std::size_t frame = 0; frame < kFrames; ++frame) {
        if (buffer[0][frame] != 0 || buffer[1][frame] != 0)
            return frame;
    }
    return kFrames;
}

struct metrics {
    double reference_rms = 0.0;
    double candidate_rms = 0.0;
    double rms_ratio = 0.0;
    double optimal_gain = 0.0;
    double gain_matched_nrmse = 0.0;
    double zero_lag_correlation = 0.0;
    double best_correlation = -1.0;
    int best_lag = 0;
};

double correlation_at_lag(
    const stereo_buffer& reference,
    const stereo_buffer& candidate,
    std::size_t begin,
    std::size_t end,
    int lag) {
    long double sum_xy = 0.0L;
    long double sum_x2 = 0.0L;
    long double sum_y2 = 0.0L;

    for (std::size_t frame = begin; frame < end; ++frame) {
        const auto shifted = static_cast<std::int64_t>(frame) + static_cast<std::int64_t>(lag);
        if (shifted < static_cast<std::int64_t>(begin)
            || shifted >= static_cast<std::int64_t>(end))
            continue;
        const std::size_t candidate_frame = static_cast<std::size_t>(shifted);
        for (std::size_t plane = 0; plane < 2u; ++plane) {
            const long double x = static_cast<long double>(reference[plane][frame]);
            const long double y = static_cast<long double>(candidate[plane][candidate_frame]);
            sum_xy += x * y;
            sum_x2 += x * x;
            sum_y2 += y * y;
        }
    }

    if (sum_x2 == 0.0L || sum_y2 == 0.0L)
        return 0.0;
    return static_cast<double>(sum_xy / std::sqrt(sum_x2 * sum_y2));
}

metrics measure_window(
    const stereo_buffer& reference,
    const stereo_buffer& candidate,
    std::size_t begin,
    std::size_t end) {
    metrics result{};
    if (begin >= end || end > kFrames)
        return result;

    long double ref_energy = 0.0L;
    long double cand_energy = 0.0L;
    long double dot = 0.0L;
    std::size_t sample_count = 0u;
    for (std::size_t frame = begin; frame < end; ++frame) {
        for (std::size_t plane = 0; plane < 2u; ++plane) {
            const long double x = static_cast<long double>(reference[plane][frame]);
            const long double y = static_cast<long double>(candidate[plane][frame]);
            ref_energy += x * x;
            cand_energy += y * y;
            dot += x * y;
            ++sample_count;
        }
    }

    if (sample_count == 0u)
        return result;
    result.reference_rms = std::sqrt(static_cast<double>(ref_energy / sample_count));
    result.candidate_rms = std::sqrt(static_cast<double>(cand_energy / sample_count));
    if (result.reference_rms > 0.0)
        result.rms_ratio = result.candidate_rms / result.reference_rms;
    if (cand_energy > 0.0L)
        result.optimal_gain = static_cast<double>(dot / cand_energy);

    long double error_energy = 0.0L;
    for (std::size_t frame = begin; frame < end; ++frame) {
        for (std::size_t plane = 0; plane < 2u; ++plane) {
            const long double x = static_cast<long double>(reference[plane][frame]);
            const long double y = static_cast<long double>(candidate[plane][frame])
                * static_cast<long double>(result.optimal_gain);
            const long double error = y - x;
            error_energy += error * error;
        }
    }
    if (ref_energy > 0.0L)
        result.gain_matched_nrmse = std::sqrt(static_cast<double>(error_energy / ref_energy));

    result.zero_lag_correlation = correlation_at_lag(reference, candidate, begin, end, 0);
    for (int lag = -kMaxCorrelationLag; lag <= kMaxCorrelationLag; ++lag) {
        const double correlation = correlation_at_lag(reference, candidate, begin, end, lag);
        if (correlation > result.best_correlation) {
            result.best_correlation = correlation;
            result.best_lag = lag;
        }
    }
    return result;
}

void print_metrics(std::string_view fixture, std::string_view window, const metrics& value) {
    std::cout << "fixture=" << fixture
              << " window=" << window
              << " ref_rms=" << value.reference_rms
              << " candidate_rms=" << value.candidate_rms
              << " rms_ratio=" << value.rms_ratio
              << " optimal_gain=" << value.optimal_gain
              << " gain_matched_nrmse=" << value.gain_matched_nrmse
              << " zero_lag_corr=" << value.zero_lag_correlation
              << " best_corr=" << value.best_correlation
              << " best_lag=" << value.best_lag
              << '\n';
}

} // namespace

int main() {
    try {
        opm_instance reference(devDef_YM2151_MAME);
        opm_instance candidate(devDef_YM2151_Nuked);

        std::cout << std::fixed << std::setprecision(9);
        for (const fixture_id fixture : kFixtures) {
            const stereo_buffer reference_first = run_fixture(reference, fixture);
            const stereo_buffer reference_repeat = run_fixture(reference, fixture);
            const stereo_buffer candidate_first = run_fixture(candidate, fixture);
            const stereo_buffer candidate_repeat = run_fixture(candidate, fixture);

            if (!buffers_equal(reference_first, reference_repeat)) {
                std::cerr << fixture_name(fixture)
                          << ": MAME output was not deterministic across reset/replay\n";
                return 2;
            }
            if (!buffers_equal(candidate_first, candidate_repeat)) {
                std::cerr << fixture_name(fixture)
                          << ": Nuked-OPM output was not deterministic across reset/replay\n";
                return 3;
            }

            const std::size_t ref_start = first_audible_frame(reference_first);
            const std::size_t candidate_start = first_audible_frame(candidate_first);
            if (ref_start == kFrames || candidate_start == kFrames) {
                std::cerr << fixture_name(fixture)
                          << ": one renderer produced no audible output\n";
                return 4;
            }

            const std::size_t transient_begin = std::min(ref_start, candidate_start);
            const std::size_t transient_end = std::min(kFrames, transient_begin + kTransientFrames);
            const std::size_t steady_begin = kFrames - kSteadyFrames;

            std::cout << "fixture=" << fixture_name(fixture)
                      << " reference_first_audible=" << ref_start
                      << " candidate_first_audible=" << candidate_start
                      << '\n';
            print_metrics(
                fixture_name(fixture),
                "transient",
                measure_window(reference_first, candidate_first, transient_begin, transient_end));
            print_metrics(
                fixture_name(fixture),
                "steady",
                measure_window(reference_first, candidate_first, steady_begin, kFrames));
            print_metrics(
                fixture_name(fixture),
                "whole",
                measure_window(reference_first, candidate_first, 0u, kFrames));
        }

        std::cout
            << "diagnostic only: renderer differences measured; no quality admission threshold applied\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "YM2151 renderer pair probe setup failed: " << error.what() << '\n';
        return 5;
    }
}
