#include "../../components/vgm/enhancement/sn76489_enhanced.h"

#include <array>
#include <cmath>
#include <cstddef>

using gameaudio::vgm::sn76489_enhanced;
using gameaudio::vgm::sn76489_timed_write;
using gameaudio::vgm::sn76489_write_kind;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

constexpr std::size_t frames = 4096;

using buffer = std::array<float, frames>;

void set_tone_period(sn76489_enhanced& psg, std::size_t channel, std::uint16_t period) {
    const std::uint8_t latch = static_cast<std::uint8_t>(
        0x80u | ((channel & 0x03u) << 5) | (period & 0x0Fu));
    psg.write(latch);
    psg.write(static_cast<std::uint8_t>((period >> 4) & 0x3Fu));
}

void set_volume(sn76489_enhanced& psg, std::size_t channel, std::uint8_t attenuation) {
    psg.write(static_cast<std::uint8_t>(
        0x90u | ((channel & 0x03u) << 5) | (attenuation & 0x0Fu)));
}

double rms(const float* data, std::size_t count) {
    double sum = 0.0;
    for (std::size_t i = 0; i < count; ++i)
        sum += static_cast<double>(data[i]) * static_cast<double>(data[i]);
    return std::sqrt(sum / static_cast<double>(count));
}

double rms(const buffer& data) {
    return rms(data.data(), data.size());
}

std::size_t negative_crossings(const buffer& data) {
    std::size_t count = 0;
    for (std::size_t i = 1; i < data.size(); ++i) {
        if (data[i - 1] >= 0.0f && data[i] < 0.0f)
            ++count;
    }
    return count;
}

} // namespace

int main() {
    sn76489_enhanced::config cfg;
    cfg.chip_clock_hz = 3579545.0;
    cfg.sample_rate_hz = 48000.0;
    cfg.white_noise_feedback = 0x0009;
    cfg.shift_register_width = 16;
    cfg.oversample = 4;

    sn76489_enhanced psg(cfg);

    CHECK(psg.tone_period(0) == 1);
    CHECK(psg.attenuation(0) == 15);
    CHECK(psg.noise_lfsr() == 0x8000);

    // One audible tone must remain isolated from the other three source stems.
    set_tone_period(psg, 0, 0x100);
    set_volume(psg, 0, 0);

    buffer tone{};
    buffer tone1{};
    buffer tone2{};
    buffer noise{};
    float* outputs[sn76489_enhanced::stem_count] = {
        tone.data(), tone1.data(), tone2.data(), noise.data()
    };
    psg.render(outputs, frames);

    CHECK(rms(tone) > 0.5);
    CHECK(rms(tone1) == 0.0);
    CHECK(rms(tone2) == 0.0);
    CHECK(rms(noise) == 0.0);

    // N=256 at the NTSC master clock is about 436.96 Hz. The crossing count
    // over this window checks that the enhanced oscillator follows the encoded
    // PSG pitch rather than a guessed musical note.
    const double seconds = static_cast<double>(frames) / cfg.sample_rate_hz;
    const double expected_cycles = (cfg.chip_clock_hz / (32.0 * 256.0)) * seconds;
    const double observed_cycles = static_cast<double>(negative_crossings(tone));
    CHECK(std::abs(observed_cycles - expected_cycles) < 2.0);

    // The encoded attenuation ladder must stay exactly 2 dB/step in floating
    // point, without the integer rounding of the historical mixer table.
    sn76489_enhanced full(cfg);
    sn76489_enhanced minus_two_db(cfg);
    set_tone_period(full, 0, 0x100);
    set_tone_period(minus_two_db, 0, 0x100);
    set_volume(full, 0, 0);
    set_volume(minus_two_db, 0, 1);

    buffer full_buffer{};
    buffer attenuated_buffer{};
    float* full_outputs[sn76489_enhanced::stem_count] = {full_buffer.data(), nullptr, nullptr, nullptr};
    float* attenuated_outputs[sn76489_enhanced::stem_count] = {attenuated_buffer.data(), nullptr, nullptr, nullptr};
    full.render(full_outputs, frames);
    minus_two_db.render(attenuated_outputs, frames);

    const double ratio = rms(attenuated_buffer) / rms(full_buffer);
    CHECK(std::abs(ratio - std::pow(10.0, -0.1)) < 1e-5);

    // White-noise writes reset the LFSR exactly as the source chip does. The
    // generated noise then evolves deterministically from that source state.
    sn76489_enhanced noise_psg(cfg);
    noise_psg.write(0xE4); // noise register: white noise, fixed rate 0
    set_volume(noise_psg, 3, 0);
    CHECK(noise_psg.noise_lfsr() == 0x8000);

    buffer white_noise{};
    float* noise_outputs[sn76489_enhanced::stem_count] = {nullptr, nullptr, nullptr, white_noise.data()};
    noise_psg.render(noise_outputs, frames);
    CHECK(rms(white_noise) > 0.1);
    CHECK(noise_psg.noise_lfsr() != 0x8000);

    noise_psg.write(0xE4);
    CHECK(noise_psg.noise_lfsr() == 0x8000);

    // Noise mode 3 derives its clock from tone channel 3 in realtime.
    set_tone_period(noise_psg, 2, 0x180);
    noise_psg.write(0xE7); // white noise, tone-3-derived clock
    CHECK((noise_psg.noise_control() & 0x03u) == 3);

    // Timed writes must land inside the block, not at the 10 ms foobar block
    // boundary. Silence channel 0 exactly halfway through the rendered block.
    sn76489_enhanced timed(cfg);
    set_tone_period(timed, 0, 0x100);
    set_volume(timed, 0, 0);
    const sn76489_timed_write timed_writes[] = {
        {frames / 2, sn76489_write_kind::register_write, 0x9F}
    };
    buffer timed_buffer{};
    float* timed_outputs[sn76489_enhanced::stem_count] = {timed_buffer.data(), nullptr, nullptr, nullptr};
    timed.render_timed(timed_writes, 1, timed_outputs, frames);
    CHECK(rms(timed_buffer.data(), frames / 2) > 0.5);
    CHECK(rms(timed_buffer.data() + frames / 2, frames / 2) == 0.0);

    // A write at exactly the end of the block changes only following state.
    const sn76489_timed_write end_write[] = {
        {frames, sn76489_write_kind::register_write, 0x90}
    };
    timed.reset();
    set_tone_period(timed, 0, 0x100);
    set_volume(timed, 0, 15);
    timed_buffer.fill(0.0f);
    timed.render_timed(end_write, 1, timed_outputs, frames);
    CHECK(rms(timed_buffer) == 0.0);
    CHECK(timed.attenuation(0) == 0);

    return 0;
}
