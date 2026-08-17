#include "../../components/vgm/enhancement/ym2612_pcm_stream.h"
#include "../../components/vgm/enhancement/ym2612_pcm_source_queue.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

using gameaudio::vgm::dac_stream_source_event;
using gameaudio::vgm::dac_stream_source_event_kind;
using gameaudio::vgm::ym2612_pcm_source_frame;
using gameaudio::vgm::ym2612_pcm_source_queue;
using gameaudio::vgm::ym2612_pcm_stream;
using gameaudio::vgm::ym2612_pcm_stream_bank;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

float level(std::uint8_t value) {
    return static_cast<float>(static_cast<int>(value) - 128) / 128.0f;
}

dac_stream_source_event event(
    dac_stream_source_event_kind kind,
    const std::uint8_t* data,
    std::size_t data_length,
    std::uint32_t frequency,
    std::uint8_t step_size = 1,
    std::uint8_t step_base = 0,
    std::uint8_t play_mode = 0,
    std::uint32_t start = 0,
    std::uint32_t length = 0) {
    dac_stream_source_event out;
    out.kind = kind;
    out.data = data;
    out.data_length = data_length;
    out.frequency = frequency;
    out.step_size = step_size;
    out.step_base = step_base;
    out.play_mode = play_mode;
    out.start_offset = start;
    out.length = length;
    return out;
}

dac_stream_source_event bank_event(
    dac_stream_source_event_kind kind,
    std::uint8_t stream_id,
    const std::uint8_t* data,
    std::size_t data_length,
    std::uint32_t frequency = 48000,
    std::uint32_t length = 2) {
    auto out = event(kind, data, data_length, frequency, 1, 0, 0x01, 0, length);
    out.stream_id = stream_id;
    out.chip_type = 0x02;
    out.chip_id = 0;
    out.chip_command = 0x002A;
    out.bank_id = 0;
    return out;
}

void prepare_bank_stream(
    ym2612_pcm_stream_bank& bank,
    std::uint8_t stream_id,
    const std::uint8_t* data,
    std::size_t data_length) {
    bank.apply(bank_event(dac_stream_source_event_kind::setup, stream_id, data, data_length));
    bank.apply(bank_event(dac_stream_source_event_kind::set_data, stream_id, data, data_length));
    bank.apply(bank_event(dac_stream_source_event_kind::set_frequency, stream_id, data, data_length));
    bank.apply(bank_event(dac_stream_source_event_kind::start, stream_id, data, data_length));
}

} // namespace

int main() {
    const std::array<std::uint8_t, 6> data{{0x00, 0x40, 0x80, 0xC0, 0xFF, 0x20}};

    // At equal source/output rates, integer source positions must survive the
    // windowed-sinc renderer exactly. This guards against a "hi-fi" resampler
    // that subtly rewrites the source samples themselves.
    ym2612_pcm_stream stream(48000.0);
    stream.apply(event(dac_stream_source_event_kind::set_data, data.data(), data.size(), 0));
    stream.apply(event(dac_stream_source_event_kind::set_frequency, data.data(), data.size(), 48000));
    stream.apply(event(dac_stream_source_event_kind::start, data.data(), data.size(), 48000, 1, 0, 0x01, 0, 5));
    CHECK(stream.valid());
    CHECK(stream.active());
    CHECK(stream.command_count() == 5);

    std::array<float, 6> output{};
    stream.render(output.data(), output.size());
    for (std::size_t i = 0; i < 5; ++i)
        CHECK(std::abs(output[i] - level(data[i])) < 1e-6f);
    CHECK(output[5] == 0.0f);
    CHECK(!stream.active());

    // Reverse mode walks the same encoded source rather than generating a new
    // reversed buffer or changing its contents.
    stream.apply(event(dac_stream_source_event_kind::start, data.data(), data.size(), 48000, 1, 0, 0x11, 0, 5));
    output.fill(0.0f);
    stream.render(output.data(), 5);
    CHECK(std::abs(output[0] - level(data[4])) < 1e-6f);
    CHECK(std::abs(output[1] - level(data[3])) < 1e-6f);
    CHECK(std::abs(output[4] - level(data[0])) < 1e-6f);

    // Loop mode wraps logical source commands without discontinuing source time.
    stream.apply(event(dac_stream_source_event_kind::start, data.data(), data.size(), 48000, 1, 0, 0x81, 0, 3));
    output.fill(0.0f);
    stream.render(output.data(), output.size());
    CHECK(std::abs(output[0] - level(data[0])) < 1e-6f);
    CHECK(std::abs(output[1] - level(data[1])) < 1e-6f);
    CHECK(std::abs(output[2] - level(data[2])) < 1e-6f);
    CHECK(std::abs(output[3] - level(data[0])) < 1e-6f);
    CHECK(std::abs(output[4] - level(data[1])) < 1e-6f);
    CHECK(stream.active());

    // Interleaved VGM streams use step size/base, e.g. base 1 of a two-byte
    // interleave selects source indices 1,3,5 without deinterleaving storage.
    stream.apply(event(dac_stream_source_event_kind::set_data, data.data(), data.size(), 48000, 2, 1));
    stream.apply(event(dac_stream_source_event_kind::start, data.data(), data.size(), 48000, 2, 1, 0x01, 0, 3));
    output.fill(0.0f);
    stream.render(output.data(), 3);
    CHECK(std::abs(output[0] - level(data[1])) < 1e-6f);
    CHECK(std::abs(output[1] - level(data[3])) < 1e-6f);
    CHECK(std::abs(output[2] - level(data[5])) < 1e-6f);

    // Length mode 02 is milliseconds per the VGM specification. At 1000 Hz,
    // 4 ms is exactly four source commands.
    stream.reset();
    stream.configure_output_rate(1000.0);
    stream.apply(event(dac_stream_source_event_kind::set_data, data.data(), data.size(), 0));
    stream.apply(event(dac_stream_source_event_kind::set_frequency, data.data(), data.size(), 1000));
    stream.apply(event(dac_stream_source_event_kind::start, data.data(), data.size(), 1000, 1, 0, 0x02, 0, 4));
    CHECK(stream.command_count() == 4);

    // A start offset of -1 keeps the previously configured source start address
    // and restarts from that address instead of indexing off the end of the bank.
    stream.advance(1);
    dac_stream_source_event ignored_start = event(
        dac_stream_source_event_kind::start, data.data(), data.size(), 1000,
        1, 0, 0x00, std::numeric_limits<std::uint32_t>::max(), 0);
    stream.apply(ignored_start);
    CHECK(stream.valid());
    CHECK(stream.command_count() == 4);
    CHECK(stream.position() == 0.0);

    // The engine-clock queue is the common bridge used by ordinary blocks and
    // deferred FM render-ahead. It keeps exact host ordinals and marks only the
    // frames whose source-bank stream unambiguously owns YM2612 register $2A.
    const std::array<std::uint8_t, 2> queue_data{{0xC0, 0x40}};
    ym2612_pcm_stream_bank bank(48000.0);
    prepare_bank_stream(bank, 7, queue_data.data(), queue_data.size());

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

    // Ownership disappears on the first frame after the authored stream ends.
    // A direct/reference DAC source therefore remains untouched at ordinal 102.
    CHECK(queue.pop_expected(102, frame));
    CHECK(!frame.replace_reference);
    CHECK(frame.left == 0);
    CHECK(frame.right == 0);

    // DAC disable suppresses replacement without freezing source-bank time.
    prepare_bank_stream(bank, 7, queue_data.data(), queue_data.size());
    queue.reset(200);
    CHECK(queue.render_until(bank, 202, false, true, true, 1, 1));
    CHECK(queue.pop_expected(200, frame));
    CHECK(!frame.replace_reference);
    CHECK(queue.pop_expected(201, frame));
    CHECK(!frame.replace_reference);
    CHECK(bank.active_target_count() == 0);

    // Two simultaneous streams claiming one YM2612 $2A identity are ambiguous.
    // Do not sum them and call that source-faithful: fail this family closed.
    bank.reset();
    prepare_bank_stream(bank, 1, queue_data.data(), queue_data.size());
    prepare_bank_stream(bank, 2, queue_data.data(), queue_data.size());
    queue.reset(300);
    CHECK(!queue.render_until(bank, 301, true, true, true, 1, 1));
    CHECK(!queue.valid());
    CHECK(queue.size() == 0);

    // Producer time may never rewind behind the queue's next host ordinal.
    bank.reset();
    queue.reset(400);
    CHECK(!queue.render_until(bank, 399, true, true, true, 1, 1));
    CHECK(!queue.valid());

    return 0;
}
