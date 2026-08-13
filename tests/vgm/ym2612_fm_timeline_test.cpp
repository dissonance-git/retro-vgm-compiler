#include "../../components/vgm/enhancement/ym2612_fm_timeline.h"

#include <array>
#include <cstddef>
#include <cstdint>

using gameaudio::vgm::ym2612_fm_backend;
using gameaudio::vgm::ym2612_fm_backend_config;
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
        rendered_frames = 0;
        write_count = 0;
        state_value = 0;
    }

    void write(std::uint8_t port, std::uint8_t reg, std::uint8_t data) noexcept override {
        ++write_count;
        last_port = port;
        last_reg = reg;
        last_data = data;
        state_value = static_cast<int>(data);
    }

    void render(float* const outputs[channel_count], std::size_t frames) noexcept override {
        for (std::size_t frame = 0; frame < frames; ++frame) {
            for (std::size_t channel = 0; channel < channel_count; ++channel) {
                if (outputs[channel] != nullptr)
                    outputs[channel][frame] = static_cast<float>(state_value + static_cast<int>(channel));
            }
        }
        rendered_frames += frames;
    }

    bool configured = false;
    ym2612_fm_backend_config last_config{};
    std::size_t rendered_frames = 0;
    std::size_t write_count = 0;
    std::uint8_t last_port = 0;
    std::uint8_t last_reg = 0;
    std::uint8_t last_data = 0;
    int state_value = 0;
};

} // namespace

int main() {
    fake_backend backend;

    // Clock ownership is explicit at the backend boundary. The timeline stays
    // in consumer/output frames while a mature OPN2 implementation may run at
    // a different native synthesis rate internally.
    const ym2612_fm_backend_config invalid_config{};
    CHECK(!invalid_config.valid());
    CHECK(!backend.configure(invalid_config));

    const ym2612_fm_backend_config config{7670454, 48000};
    CHECK(config.valid());
    CHECK(backend.configure(config));
    CHECK(backend.configured);
    CHECK(backend.last_config.chip_clock_hz == 7670454);
    CHECK(backend.last_config.output_sample_rate_hz == 48000);

    ym2612_fm_timeline timeline(backend);

    constexpr std::size_t frames = 16;
    const ym2612_timed_write writes[] = {
        {4, 0, 0x30, 10},
        {8, 1, 0xB4, 20},
        {8, 0, 0x28, 30},
        {12, 0, 0x22, 40},
    };

    std::array<std::array<float, frames>, ym2612_fm_backend::channel_count> stems{};
    float* outputs[ym2612_fm_backend::channel_count]{};
    for (std::size_t channel = 0; channel < ym2612_fm_backend::channel_count; ++channel)
        outputs[channel] = stems[channel].data();

    timeline.render_timed(writes, std::size(writes), outputs, frames);

    CHECK(backend.rendered_frames == frames);
    CHECK(backend.write_count == 4);
    CHECK(backend.last_port == 0);
    CHECK(backend.last_reg == 0x22);
    CHECK(backend.last_data == 40);

    // Before the first write, the fake backend exposes state value 0.
    for (std::size_t frame = 0; frame < 4; ++frame)
        CHECK(stems[0][frame] == 0.0f);

    // First write takes effect exactly at sample 4.
    for (std::size_t frame = 4; frame < 8; ++frame)
        CHECK(stems[0][frame] == 10.0f);

    // Two writes at sample 8 are both applied before more synthesis happens.
    // The second same-sample write therefore wins for the following segment.
    for (std::size_t frame = 8; frame < 12; ++frame)
        CHECK(stems[0][frame] == 30.0f);

    for (std::size_t frame = 12; frame < frames; ++frame)
        CHECK(stems[0][frame] == 40.0f);

    // Channel identity remains separate all the way through the timing layer.
    CHECK(stems[5][6] == 15.0f);
    CHECK(stems[5][10] == 35.0f);

    // Shadow advance must execute the same writes and consume the same amount
    // of musical time even when no audio buffers are requested.
    backend.reset();
    timeline.advance_timed(writes, std::size(writes), frames);
    CHECK(backend.rendered_frames == frames);
    CHECK(backend.write_count == 4);
    CHECK(backend.state_value == 40);

    // Out-of-order writes are ignored rather than rewinding synthesis time.
    backend.reset();
    const ym2612_timed_write out_of_order[] = {
        {6, 0, 0x30, 1},
        {3, 0, 0x30, 99},
        {9, 0, 0x30, 2},
    };
    timeline.advance_timed(out_of_order, std::size(out_of_order), 12);
    CHECK(backend.rendered_frames == 12);
    CHECK(backend.write_count == 2);
    CHECK(backend.state_value == 2);

    return 0;
}