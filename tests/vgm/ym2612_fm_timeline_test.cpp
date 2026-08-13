#include "../../components/vgm/enhancement/ym2612_fm_clock.h"
#include "../../components/vgm/enhancement/ym2612_fm_timeline.h"

#include <array>
#include <cstddef>
#include <cstdint>

using gameaudio::vgm::vgm_timeline_tick_rate_hz;
using gameaudio::vgm::ym2612_fm_backend;
using gameaudio::vgm::ym2612_fm_backend_config;
using gameaudio::vgm::ym2612_fm_clock;
using gameaudio::vgm::ym2612_fm_clock_config;
using gameaudio::vgm::ym2612_fm_timeline;
using gameaudio::vgm::ym2612_timed_write;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

class fake_backend final : public ym2612_fm_backend {
public:
    bool configure(const ym2612_fm_backend_config& config) noexcept override {
        configured = config.valid();
        last_config = config;
        return configured;
    }

    void reset() noexcept override {
        calls = 0;
        last_write_count = 0;
        last_frames = 0;
        all_outputs_null = false;
    }

    std::size_t latency_frames() const noexcept override { return 23; }

    void render_timed(
        const ym2612_timed_write* writes,
        std::size_t write_count,
        float* const outputs[channel_count],
        std::size_t frames) noexcept override {
        ++calls;
        last_write_count = write_count;
        last_frames = frames;
        all_outputs_null = true;
        for (std::size_t channel = 0; channel < channel_count; ++channel) {
            all_outputs_null = all_outputs_null && outputs[channel] == nullptr;
            if (outputs[channel] != nullptr) {
                for (std::size_t frame = 0; frame < frames; ++frame)
                    outputs[channel][frame] = static_cast<float>(channel);
            }
        }
        for (std::size_t index = 0; index < write_count && index < seen.size(); ++index)
            seen[index] = writes[index];
    }

    bool configured = false;
    ym2612_fm_backend_config last_config{};
    std::size_t calls = 0;
    std::size_t last_write_count = 0;
    std::size_t last_frames = 0;
    bool all_outputs_null = false;
    std::array<ym2612_timed_write, 8> seen{};
};

} // namespace

int main() {
    constexpr std::uint32_t sonic_clock = 7670453;

    ym2612_fm_clock clock;
    CHECK(!clock.configure({}));
    CHECK(clock.configure(ym2612_fm_clock_config{sonic_clock, vgm_timeline_tick_rate_hz}));
    CHECK(clock.native_sample_at_or_after_tick(0) == 0);
    CHECK(clock.native_sample_at_or_after_tick(1) == 2);
    CHECK(clock.native_sample_at_or_after_tick(2) == 3);
    CHECK(clock.native_sample_at_or_after_tick(44100) == 53268);

    CHECK(clock.configure(ym2612_fm_clock_config{6350400, 44100}));
    CHECK(clock.native_sample_at_or_after_tick(1) == 1);
    CHECK(clock.native_sample_at_or_after_tick(44100) == 44100);

    CHECK(clock.configure(ym2612_fm_clock_config{sonic_clock, 44100}));
    constexpr std::uint64_t ten_hours_ticks = 44100ull * 60ull * 60ull * 10ull;
    CHECK(clock.native_sample_at_or_after_tick(ten_hours_ticks) == 1917613250ull);

    fake_backend backend;
    const ym2612_fm_backend_config invalid_config{};
    CHECK(!backend.configure(invalid_config));
    const ym2612_fm_backend_config config{sonic_clock, 44100, 96000};
    CHECK(backend.configure(config));
    CHECK(backend.last_config.source_tick_rate_hz == 44100);
    CHECK(backend.last_config.output_sample_rate_hz == 96000);
    CHECK(backend.latency_frames() == 23);

    ym2612_fm_timeline timeline(backend);
    const ym2612_timed_write writes[] = {
        {100, 0, 0x30, 10},
        {101, 1, 0xB4, 20},
        {101, 0, 0x28, 30},
        {105, 0, 0x22, 40},
    };

    constexpr std::size_t frames = 16;
    std::array<std::array<float, frames>, ym2612_fm_backend::channel_count> stems{};
    float* outputs[ym2612_fm_backend::channel_count]{};
    for (std::size_t channel = 0; channel < ym2612_fm_backend::channel_count; ++channel)
        outputs[channel] = stems[channel].data();

    timeline.render_timed(writes, std::size(writes), outputs, frames);
    CHECK(backend.calls == 1);
    CHECK(backend.last_write_count == 4);
    CHECK(backend.last_frames == frames);
    CHECK(!backend.all_outputs_null);
    CHECK(backend.seen[0].tick == 100);
    CHECK(backend.seen[1].tick == 101);
    CHECK(backend.seen[2].tick == 101);
    CHECK(backend.seen[1].data == 20);
    CHECK(backend.seen[2].data == 30);
    CHECK(stems[5][10] == 5.0f);

    // Shadow advancement must use the exact same whole-block backend path.
    timeline.advance_timed(writes, std::size(writes), frames);
    CHECK(backend.calls == 2);
    CHECK(backend.all_outputs_null);
    CHECK(backend.last_write_count == 4);
    CHECK(backend.seen[3].tick == 105);

    return 0;
}
