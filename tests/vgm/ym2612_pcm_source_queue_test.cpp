#include "components/vgm/enhancement/ym2612_pcm_source_queue.h"

#include <array>
#include <cstddef>
#include <cstdint>

using gameaudio::vgm::dac_stream_source_event;
using gameaudio::vgm::dac_stream_source_event_kind;
using gameaudio::vgm::ym2612_pcm_source_frame;
using gameaudio::vgm::ym2612_pcm_source_queue;
using gameaudio::vgm::ym2612_pcm_stream_bank;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

dac_stream_source_event stream_event(
    dac_stream_source_event_kind kind,
    std::uint8_t stream_id,
    const std::uint8_t* data,
    std::size_t data_length,
    std::uint32_t frequency = 48000,
    std::uint32_t length = 2) {
    dac_stream_source_event event{};
    event.kind = kind;
    event.stream_id = stream_id;
    event.chip_type = 0x02;
    event.chip_id = 0;
    event.chip_command = 0x002A;
    event.bank_id = 0;
    event.step_size = 1;
    event.step_base = 0;
    event.play_mode = 0x01;
    event.frequency = frequency;
    event.start_offset = 0;
    event.length = length;
    event.data = data;
    event.data_length = data_length;
    return event;
}

void prepare_stream(
    ym2612_pcm_stream_bank& bank,
    std::uint8_t stream_id,
    const std::uint8_t* data,
    std::size_t data_length) {
    bank.apply(stream_event(dac_stream_source_event_kind::setup, stream_id, data, data_length));
    bank.apply(stream_event(dac_stream_source_event_kind::set_data, stream_id, data, data_length));
    bank.apply(stream_event(dac_stream_source_event_kind::set_frequency, stream_id, data, data_length));
    bank.apply(stream_event(dac_stream_source_event_kind::start, stream_id, data, data_length));
}

} // namespace

int main() {
    const std::array<std::uint8_t, 2> data{{0xC0, 0x40}};

    ym2612_pcm_stream_bank bank(48000.0);
    prepare_stream(bank, 7, data.data(), data.size());

    ym2612_pcm_source_queue<8> queue;
    queue.reset(100);
    CHECK(queue.render_until(bank, 103, true, true, false, 1, 1));
    CHECK(queue.valid());
    CHECK(queue.next_ordinal() == 103);
    CHECK(queue.size() == 3);

    ym2612_pcm_source_frame frame{};
    CHECK(queue.pop_expected(100, frame));
    CHECK(frame.replace_reference);
    CHECK(frame.left == 4224);
    CHECK(frame.right == 0);

    CHECK(queue.pop_expected(101, frame));
    CHECK(frame.replace_reference);
    CHECK(frame.left == -4224);
    CHECK(frame.right == 0);

    // The source stream has ended. Ownership disappears on the exact next host
    // frame, so a direct/reference DAC source can remain untouched there.
    CHECK(queue.pop_expected(102, frame));
    CHECK(!frame.replace_reference);
    CHECK(frame.left == 0);
    CHECK(frame.right == 0);

    // DAC disable suppresses replacement without freezing source-bank time.
    prepare_stream(bank, 7, data.data(), data.size());
    queue.reset(200);
    CHECK(queue.render_until(bank, 202, false, true, true, 1, 1));
    CHECK(queue.pop_expected(200, frame));
    CHECK(!frame.replace_reference);
    CHECK(queue.pop_expected(201, frame));
    CHECK(!frame.replace_reference);
    CHECK(bank.active_target_count() == 0);

    // Ambiguous simultaneous ownership is evidence failure. Do not mix two
    // dac_control streams into one YM2612 $2A identity and call it faithful.
    bank.reset();
    prepare_stream(bank, 1, data.data(), data.size());
    prepare_stream(bank, 2, data.data(), data.size());
    queue.reset(300);
    CHECK(!queue.render_until(bank, 301, true, true, true, 1, 1));
    CHECK(!queue.valid());
    CHECK(queue.size() == 0);

    // Ordinals are monotonic. Rewinding the producer would make source identity
    // ambiguous, so it fails closed instead of silently replaying frames.
    bank.reset();
    queue.reset(400);
    CHECK(!queue.render_until(bank, 399, true, true, true, 1, 1));
    CHECK(!queue.valid());

    return 0;
}
