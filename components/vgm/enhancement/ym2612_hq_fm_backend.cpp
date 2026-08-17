#include "ym2612_hq_fm_backend.h"

#include "yamaha_opn_register.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gameaudio::vgm {

namespace {

constexpr double pi = 3.141592653589793238462643383279502884;
constexpr double two_pi = 2.0 * pi;
constexpr double hardware_operator_peak = 8191.0;
constexpr double hardware_phase_units = 1024.0;
constexpr double phase_mod_radians_per_unit = two_pi / hardware_phase_units;
constexpr double phase_accumulator_scale = 1048576.0; // 2^20

// This is the exact OPN2 operator-routing table used by Nuked OPN2. Keeping the
// graph table while changing the arithmetic ceiling is the central distinction
// between a source-native FM descendant and a preset substitution.
constexpr std::uint8_t fm_algorithm[4][6][8] = {
    {
        {1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,1},
    },
    {
        {0,1,0,0,0,1,0,0},
        {0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,1},
    },
    {
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {1,0,0,1,1,1,1,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,1},
    },
    {
        {0,0,1,0,0,1,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,1,0,0,0,0},
        {1,1,0,1,1,0,0,0},
        {0,0,1,0,0,0,0,0},
        {1,1,1,1,1,1,1,1},
    },
};

constexpr std::uint8_t fn_note[16] = {
    0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3,
};
constexpr std::uint32_t pg_detune[8] = {16,17,19,20,22,24,27,29};

inline bool event_due(
    std::uint64_t tick,
    std::uint64_t internal_sample,
    std::uint32_t source_tick_rate,
    double internal_rate) noexcept
{
    const long double event_time = static_cast<long double>(tick)
        / static_cast<long double>(source_tick_rate);
    const long double sample_time = static_cast<long double>(internal_sample)
        / static_cast<long double>(internal_rate);
    return event_time <= sample_time;
}

} // namespace

ym2612_hq_fm_backend::ym2612_hq_fm_backend(ym2612_hq_fm_profile profile) noexcept
    : profile_(profile) {
    reset();
}

bool ym2612_hq_fm_backend::configure(const ym2612_fm_backend_config& config) noexcept {
    if (!config.valid() || !profile_.valid()
        || profile_.mode == ym2612_enhanced_fm_mode::reference_opn2) {
        configured_ = false;
        return false;
    }
    config_ = config;
    configured_ = true;
    reset();
    return true;
}

void ym2612_hq_fm_backend::reset() noexcept {
    channels_ = {};
    fnum_high_latch_ = {{0, 0}};
    channel3_mode_ = 0;
    dac_enabled_ = false;
    lfo_enabled_ = false;
    lfo_frequency_ = 0;
    internal_sample_cursor_ = 0;
    semantic_coverage_complete_ = true;
}

std::size_t ym2612_hq_fm_backend::channel_for_register(
    std::uint8_t port,
    std::uint8_t reg) noexcept {
    const std::uint8_t local = static_cast<std::uint8_t>(reg & 0x03u);
    if (port > 1u || local >= 3u)
        return channel_count;
    return static_cast<std::size_t>(port) * 3u + local;
}

std::size_t ym2612_hq_fm_backend::operator_for_register(std::uint8_t reg) noexcept {
    return static_cast<std::size_t>(opn_operator_from_register(reg));
}

void ym2612_hq_fm_backend::apply_key_write(std::uint8_t data) noexcept {
    const std::uint8_t local = static_cast<std::uint8_t>(data & 0x03u);
    if (local == 3u)
        return;
    std::size_t channel = local;
    if ((data & 0x04u) != 0)
        channel += 3u;
    if (channel >= channels_.size())
        return;

    auto& ch = channels_[channel];
    const std::uint8_t key_mask = static_cast<std::uint8_t>((data >> 4) & 0x0fu);
    for (std::size_t op_index = 0; op_index < ch.operators.size(); ++op_index) {
        auto& op = ch.operators[op_index];
        const bool next_key = (key_mask & (1u << op_index)) != 0;
        if (next_key && !op.keyed) {
            op.keyed = true;
            op.phase_cycles = 0.0;
            op.envelope = 0.0;
            op.stage = envelope_stage::attack;
        } else if (!next_key && op.keyed) {
            op.keyed = false;
            if (op.stage != envelope_stage::off)
                op.stage = envelope_stage::release;
        }
    }
}

void ym2612_hq_fm_backend::apply_write(const ym2612_timed_write& write) noexcept {
    if (write.port > 1u)
        return;

    if (write.port == 0) {
        switch (write.reg) {
        case 0x22:
            lfo_enabled_ = (write.data & 0x08u) != 0;
            lfo_frequency_ = static_cast<std::uint8_t>(write.data & 0x07u);
            // The first HQ backend does not yet claim exact OPN LFO phase/AM/PM
            // timing. Keep rendering a bounded candidate, but fence it out of
            // automatic replacement whenever the source actually enables LFO.
            if (lfo_enabled_)
                semantic_coverage_complete_ = false;
            return;
        case 0x27:
            channel3_mode_ = static_cast<std::uint8_t>((write.data >> 6) & 0x03u);
            if (channel3_mode_ != 0)
                semantic_coverage_complete_ = false;
            return;
        case 0x28:
            apply_key_write(write.data);
            return;
        case 0x2b:
            dac_enabled_ = (write.data & 0x80u) != 0;
            return;
        default:
            break;
        }
    }

    if (opn_operator_register(write.reg)) {
        const std::size_t channel = channel_for_register(write.port, write.reg);
        const std::size_t op_index = operator_for_register(write.reg);
        if (channel >= channels_.size() || op_index >= 4)
            return;
        auto& op = channels_[channel].operators[op_index];
        switch (write.reg & 0xf0u) {
        case 0x30:
            op.multiple = static_cast<std::uint8_t>(write.data & 0x0fu);
            op.detune = static_cast<std::uint8_t>((write.data >> 4) & 0x07u);
            return;
        case 0x40:
            op.total_level = static_cast<std::uint8_t>(write.data & 0x7fu);
            return;
        case 0x50:
            op.attack_rate = static_cast<std::uint8_t>(write.data & 0x1fu);
            op.key_scale = static_cast<std::uint8_t>((write.data >> 6) & 0x03u);
            return;
        case 0x60:
            op.decay_rate = static_cast<std::uint8_t>(write.data & 0x1fu);
            op.amplitude_modulation = (write.data & 0x80u) != 0;
            if (op.amplitude_modulation)
                semantic_coverage_complete_ = false;
            return;
        case 0x70:
            op.sustain_rate = static_cast<std::uint8_t>(write.data & 0x1fu);
            return;
        case 0x80:
            op.release_rate = static_cast<std::uint8_t>(write.data & 0x0fu);
            op.sustain_level = static_cast<std::uint8_t>((write.data >> 4) & 0x0fu);
            return;
        case 0x90:
            op.ssg_eg = static_cast<std::uint8_t>(write.data & 0x0fu);
            if (op.ssg_eg != 0)
                semantic_coverage_complete_ = false;
            return;
        default:
            return;
        }
    }

    if (opn_frequency_high_register(write.reg)) {
        fnum_high_latch_[write.port] = write.data;
        return;
    }
    if (opn_frequency_low_register(write.reg)) {
        const std::size_t channel = channel_for_register(write.port, write.reg);
        if (channel < channels_.size()) {
            const auto pitch = decode_opn_block_fnum(fnum_high_latch_[write.port], write.data);
            channels_[channel].fnum = pitch.fnum;
            channels_[channel].block = pitch.block;
        }
        return;
    }

    if (write.port == 0
        && (opn_ch3_frequency_low_register(write.reg)
            || opn_ch3_frequency_high_register(write.reg))) {
        semantic_coverage_complete_ = false;
        return;
    }

    if (opn_algorithm_feedback_register(write.reg)) {
        const std::size_t channel = channel_for_register(write.port, write.reg);
        if (channel < channels_.size()) {
            channels_[channel].algorithm = opn_algorithm(write.data);
            channels_[channel].feedback = opn_feedback(write.data);
        }
        return;
    }

    if (write.reg >= 0xb4u && write.reg <= 0xb6u) {
        const std::size_t channel = channel_for_register(write.port, write.reg);
        if (channel < channels_.size()) {
            auto& ch = channels_[channel];
            ch.pan_left = (write.data & 0x80u) != 0;
            ch.pan_right = (write.data & 0x40u) != 0;
            ch.ams = static_cast<std::uint8_t>((write.data >> 4) & 0x03u);
            ch.fms = static_cast<std::uint8_t>(write.data & 0x07u);
            if (ch.ams != 0 || ch.fms != 0)
                semantic_coverage_complete_ = false;
        }
    }
}

double ym2612_hq_fm_backend::rate_time_seconds(
    std::uint8_t rate,
    double slow_seconds,
    double fast_seconds) noexcept {
    if (rate == 0)
        return std::numeric_limits<double>::infinity();
    const double normalized = static_cast<double>(rate) / 31.0;
    return slow_seconds * std::pow(fast_seconds / slow_seconds, normalized);
}

double ym2612_hq_fm_backend::total_level_gain(std::uint8_t total_level) noexcept {
    const double attenuation_db = static_cast<double>(total_level & 0x7fu) * 0.75;
    return std::pow(10.0, -attenuation_db / 20.0);
}

double ym2612_hq_fm_backend::sustain_gain(std::uint8_t sustain_level) noexcept {
    const std::uint8_t level = static_cast<std::uint8_t>(sustain_level & 0x0fu);
    if (level == 0x0fu)
        return 0.0;
    const double attenuation_db = static_cast<double>(level) * 3.0;
    return std::pow(10.0, -attenuation_db / 20.0);
}

void ym2612_hq_fm_backend::advance_envelope(
    operator_state& op,
    double sample_rate_hz) noexcept {
    if (!(sample_rate_hz > 0.0) || !std::isfinite(sample_rate_hz))
        return;

    auto coefficient = [sample_rate_hz](double seconds) noexcept {
        if (!std::isfinite(seconds))
            return 0.0;
        if (seconds <= 0.0)
            return 1.0;
        return 1.0 - std::exp(-1.0 / (seconds * sample_rate_hz));
    };

    switch (op.stage) {
    case envelope_stage::off:
        op.envelope = 0.0;
        break;
    case envelope_stage::attack: {
        if (op.attack_rate == 0) {
            op.envelope = 0.0;
            break;
        }
        const double k = coefficient(rate_time_seconds(op.attack_rate, 8.0, 0.0008));
        op.envelope += (1.0 - op.envelope) * k;
        if (op.envelope >= 0.9995) {
            op.envelope = 1.0;
            op.stage = envelope_stage::decay;
        }
        break;
    }
    case envelope_stage::decay: {
        const double target = sustain_gain(op.sustain_level);
        const double k = coefficient(rate_time_seconds(op.decay_rate, 30.0, 0.004));
        op.envelope += (target - op.envelope) * k;
        if (op.decay_rate != 0 && op.envelope <= target + 1.0e-5) {
            op.envelope = target;
            op.stage = envelope_stage::sustain;
        }
        break;
    }
    case envelope_stage::sustain: {
        if (op.sustain_rate == 0)
            break;
        const double k = coefficient(rate_time_seconds(op.sustain_rate, 45.0, 0.008));
        op.envelope += (0.0 - op.envelope) * k;
        if (op.envelope <= 1.0e-6) {
            op.envelope = 0.0;
            op.stage = envelope_stage::off;
        }
        break;
    }
    case envelope_stage::release: {
        // RR has four bits. Expand it monotonically onto the same bounded rate
        // family without pretending this first candidate is cycle-exact EG.
        const std::uint8_t expanded = static_cast<std::uint8_t>(op.release_rate * 2u + 1u);
        const double k = coefficient(rate_time_seconds(expanded, 20.0, 0.006));
        op.envelope += (0.0 - op.envelope) * k;
        if (op.envelope <= 1.0e-6) {
            op.envelope = 0.0;
            op.stage = envelope_stage::off;
        }
        break;
    }
    }
    op.envelope = std::clamp(op.envelope, 0.0, 1.0);
}

double ym2612_hq_fm_backend::operator_frequency_hz(
    const channel_state& channel,
    const operator_state& op) const noexcept {
    if (config_.chip_clock_hz == 0 || channel.fnum == 0)
        return 0.0;

    std::uint32_t fnum = static_cast<std::uint32_t>(channel.fnum) << 1u;
    std::uint32_t basefreq = (fnum << channel.block) >> 2u;

    const std::uint8_t kcode = static_cast<std::uint8_t>(
        (channel.block << 2u) | fn_note[(channel.fnum >> 7u) & 0x0fu]);
    const std::uint8_t dt = static_cast<std::uint8_t>(op.detune & 0x07u);
    const std::uint8_t dt_l = static_cast<std::uint8_t>(dt & 0x03u);
    std::uint32_t detune = 0;
    if (dt_l != 0) {
        const std::uint8_t limited_kcode = std::min<std::uint8_t>(kcode, 0x1cu);
        const std::uint8_t block = static_cast<std::uint8_t>(limited_kcode >> 2u);
        const std::uint8_t note = static_cast<std::uint8_t>(limited_kcode & 0x03u);
        const std::uint8_t sum = static_cast<std::uint8_t>(
            block + 9u + static_cast<std::uint8_t>((dt_l == 3u) | (dt_l & 0x02u)));
        const std::uint8_t sum_h = static_cast<std::uint8_t>(sum >> 1u);
        const std::uint8_t sum_l = static_cast<std::uint8_t>(sum & 0x01u);
        const std::uint8_t shift = static_cast<std::uint8_t>(9u - sum_h);
        detune = pg_detune[(sum_l << 2u) | note] >> shift;
    }
    if ((dt & 0x04u) != 0)
        basefreq = (basefreq - detune) & 0x1ffffu;
    else
        basefreq = (basefreq + detune) & 0x1ffffu;

    const std::uint32_t multi = op.multiple == 0
        ? 1u : static_cast<std::uint32_t>(op.multiple) << 1u;
    const std::uint32_t phase_increment = ((basefreq * multi) >> 1u) & 0xfffffu;
    const double native_rate = static_cast<double>(config_.chip_clock_hz) / 144.0;
    return native_rate * static_cast<double>(phase_increment) / phase_accumulator_scale;
}

double ym2612_hq_fm_backend::render_operator(
    channel_state& channel,
    std::size_t operator_index,
    double modulation_phase_units,
    double sample_rate_hz) noexcept {
    auto& op = channel.operators[operator_index];
    advance_envelope(op, sample_rate_hz);

    const double frequency = operator_frequency_hz(channel, op);
    if (frequency > 0.0 && std::isfinite(frequency)) {
        op.phase_cycles += frequency / sample_rate_hz;
        op.phase_cycles -= std::floor(op.phase_cycles);
    }

    const double phase = two_pi * op.phase_cycles
        + modulation_phase_units * phase_mod_radians_per_unit;
    const double amplitude = hardware_operator_peak
        * total_level_gain(op.total_level)
        * op.envelope;
    const double output = std::sin(phase) * amplitude;
    return std::isfinite(output) ? output : 0.0;
}

double ym2612_hq_fm_backend::render_channel(
    std::size_t channel_index,
    double sample_rate_hz) noexcept {
    if (channel_index >= channels_.size())
        return 0.0;
    if (channel_index == 5u && dac_enabled_)
        return 0.0;

    auto& channel = channels_[channel_index];
    const std::size_t algorithm = static_cast<std::size_t>(channel.algorithm & 0x07u);
    double carrier_sum = 0.0;

    for (std::size_t op_index = 0; op_index < 4; ++op_index) {
        double mod1 = 0.0;
        double mod2 = 0.0;
        if (fm_algorithm[op_index][0][algorithm]) mod2 += channel.op1_history0;
        if (fm_algorithm[op_index][1][algorithm]) mod1 += channel.op1_history1;
        if (fm_algorithm[op_index][2][algorithm]) mod1 += channel.op2_memory;
        if (fm_algorithm[op_index][3][algorithm]) mod2 += channel.last_operator_output;
        if (fm_algorithm[op_index][4][algorithm]) mod1 += channel.last_operator_output;

        double modulation = mod1 + mod2;
        if (op_index == 0) {
            if (channel.feedback == 0)
                modulation = 0.0;
            else
                modulation /= static_cast<double>(1u << (10u - channel.feedback));
        } else {
            modulation *= 0.5;
        }

        const double output = render_operator(
            channel, op_index, modulation, sample_rate_hz);

        if (op_index == 0) {
            channel.op1_history1 = channel.op1_history0;
            channel.op1_history0 = output;
        }
        if (op_index == 2)
            channel.op2_memory = output;
        channel.last_operator_output = output;

        if (fm_algorithm[op_index][5][algorithm])
            carrier_sum += output;
    }

    // Normalize the hardware-like operator domain without clipping. The source
    // replacement shell later calibrates this HQ stem into the exact reference
    // source-gain domain before subtraction/recomposition.
    const double normalized = carrier_sum / (hardware_operator_peak * 4.0);
    return std::isfinite(normalized) ? normalized : 0.0;
}

void ym2612_hq_fm_backend::render_timed(
    const ym2612_timed_write* writes,
    std::size_t write_count,
    float* const outputs[channel_count],
    std::size_t frames) noexcept {
    if (!configured_ || frames == 0)
        return;
    if (write_count != 0 && writes == nullptr) {
        semantic_coverage_complete_ = false;
        return;
    }

    for (std::size_t channel = 0; channel < channel_count; ++channel) {
        if (outputs[channel] != nullptr)
            std::fill(outputs[channel], outputs[channel] + frames, 0.0f);
    }

    const std::size_t oversample = static_cast<std::size_t>(profile_.internal_oversample);
    const double internal_rate = static_cast<double>(config_.output_sample_rate_hz)
        * static_cast<double>(oversample);
    std::size_t write_index = 0;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        double accumulated[channel_count]{};
        for (std::size_t sub = 0; sub < oversample; ++sub) {
            while (write_index < write_count
                && event_due(
                    writes[write_index].tick,
                    internal_sample_cursor_,
                    config_.source_tick_rate_hz,
                    internal_rate)) {
                apply_write(writes[write_index]);
                ++write_index;
            }

            for (std::size_t channel = 0; channel < channel_count; ++channel)
                accumulated[channel] += render_channel(channel, internal_rate);
            ++internal_sample_cursor_;
        }

        for (std::size_t channel = 0; channel < channel_count; ++channel) {
            if (outputs[channel] != nullptr)
                outputs[channel][frame] = static_cast<float>(
                    accumulated[channel] / static_cast<double>(oversample));
        }
    }

    // Preserve terminal writes exactly at the next-block boundary without
    // incorrectly applying events that belong later in the source timeline.
    while (write_index < write_count
        && event_due(
            writes[write_index].tick,
            internal_sample_cursor_,
            config_.source_tick_rate_hz,
            internal_rate)) {
        apply_write(writes[write_index]);
        ++write_index;
    }
    if (write_index != write_count)
        semantic_coverage_complete_ = false;
}

} // namespace gameaudio::vgm
