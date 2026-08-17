#include "components/vgm/foo_input_vgm/src/studio_alignment_queue.h"
#include "components/vgm/foo_input_vgm/src/studio_source_resampler.h"
#include "components/vgm/foo_input_vgm/src/studio_source_stream.h"

#include <array>
#include <cassert>
#include <cstdint>

using namespace foobar_vgm::source_audio;

namespace {

struct protected_frame {
    std::int32_t left = 0;
    std::int32_t right = 0;
    std::int32_t dac = 0;
    std::int32_t psg = 0;
};

studio_source_phase_position position(std::uint64_t center) {
    return {
        static_cast<std::int64_t>(
            center * studio_source_resampler_kernel::phase_count + 511),
        true
    };
}

} // namespace

int main() {
    studio_alignment_queue<protected_frame, 8> queue;
    studio_source_stream<256> stream;

    const protected_frame first{100, -100, 7, 11};
    const protected_frame second{200, -200, 13, 17};
    assert(queue.push(40, position(64), first));
    assert(queue.push(41, position(65), second));

    assert(queue.size() == 2);
    assert(!queue.front_ready(stream));

    std::array<studio_stereo_sample, 128> native{};
    for (std::size_t i = 0; i < native.size(); ++i)
        native[i] = {static_cast<double>(i), -static_cast<double>(i)};

    const auto first_window = plan_studio_source_window(position(64));
    assert(first_window.valid);
    const std::size_t missing_final = static_cast<std::size_t>(first_window.final);
    assert(stream.append(0, native.data(), missing_final));
    assert(!queue.front_ready(stream));

    assert(stream.append(
        static_cast<std::uint64_t>(missing_final),
        native.data() + missing_final,
        1));
    assert(queue.front_ready(stream));

    studio_alignment_queue<protected_frame, 8>::entry released{};
    assert(queue.pop_ready(stream, released));
    assert(released.destination_ordinal == 40);
    assert(released.payload.left == first.left);
    assert(released.payload.dac == first.dac);
    assert(released.payload.psg == first.psg);

    const auto second_window = plan_studio_source_window(position(65));
    assert(second_window.valid);
    if (!queue.front_ready(stream)) {
        const std::uint64_t next = stream.next_ordinal();
        assert(next == static_cast<std::uint64_t>(second_window.final));
        assert(stream.append(next, native.data() + next, 1));
    }
    assert(queue.pop_ready(stream, released));
    assert(released.destination_ordinal == 41);
    assert(released.payload.left == second.left);
    assert(queue.empty());

    queue.reset();
    assert(queue.push(100, position(80), first));
    assert(!queue.push(102, position(81), second));
    assert(!queue.valid());

    studio_alignment_queue<protected_frame, 1> bounded;
    assert(bounded.push(0, position(64), first));
    assert(!bounded.push(1, position(65), second));
    assert(!bounded.valid());

    queue.reset();
    assert(queue.push(500, position(120), first));
    assert(queue.pop_reference_tail(released));
    assert(released.destination_ordinal == 500);
    assert(released.payload.right == first.right);
    assert(queue.empty());

    // Richer readiness proofs, such as a fresh chip's verified silent negative
    // source-time prefix, may dequeue without pretending NativeStream::contains
    // knows about that evidence. A failed proof must leave the queue untouched.
    queue.reset();
    assert(queue.push(700, position(0), second));
    assert(!queue.pop_ready_when(false, released));
    assert(queue.size() == 1);
    assert(queue.front() != nullptr);
    assert(queue.front()->destination_ordinal == 700);
    assert(queue.pop_ready_when(true, released));
    assert(released.destination_ordinal == 700);
    assert(released.payload.psg == second.psg);
    assert(queue.empty());

    return 0;
}
