#include "components/vgm/foo_input_vgm/src/studio_frame_transport.h"

#include <cassert>
#include <cstdint>
#include <limits>

using namespace foobar_vgm::source_audio;

namespace {
studio_transport_input_frame frame(
    std::uint64_t ordinal,
    std::int32_t left,
    std::int32_t right,
    std::int64_t exact_left,
    std::int64_t exact_right,
    bool await_studio) {
    studio_transport_input_frame out;
    out.destination_ordinal = ordinal;
    out.protected_left = left;
    out.protected_right = right;
    out.exact_fm_left = exact_left;
    out.exact_fm_right = exact_right;
    out.await_studio_fm = await_studio;
    return out;
}
} // namespace

int main() {
    // The queue protects whole PlayerA frames, not FM in isolation. Reference
    // frames before the first reconstructable Studio ordinal can leave at once;
    // once an awaited FM frame reaches the head, later PSG/DAC/reference content
    // must wait behind it even if later FM ordinals become ready out of order.
    studio_frame_transport<8> transport;
    transport.reset();
    assert(transport.valid());
    assert(transport.push(frame(10, 1000, -1000, 100, -100, false)));
    assert(transport.push(frame(11, 2000, -2000, 200, -200, true)));
    assert(transport.push(frame(12, 3000, -3000, 300, -300, true)));
    assert(transport.push(frame(13, 4000, -4000, 400, -400, false)));
    assert(transport.size() == 4);
    assert(transport.waiting_frames() == 2);
    assert(transport.contiguous_final_frames() == 1);

    studio_transport_output_frame output{};
    assert(transport.pop_final(output));
    assert(output.destination_ordinal == 10);
    assert(output.left == 1000 && output.right == -1000);
    assert(!output.used_studio_fm);
    assert(transport.contiguous_final_frames() == 0);

    // Promotion may arrive for a later destination first, but it cannot jump the
    // unresolved whole-frame head. This is the core anti-desynchronization rule.
    assert(transport.apply_studio_fm(12, 450, -450));
    assert(transport.waiting_frames() == 1);
    assert(transport.contiguous_final_frames() == 0);
    assert(!transport.pop_final(output));

    assert(transport.apply_studio_fm(11, 250, -250));
    assert(transport.waiting_frames() == 0);
    assert(transport.contiguous_final_frames() == 3);

    assert(transport.pop_final(output));
    assert(output.destination_ordinal == 11);
    assert(output.left == 2050 && output.right == -2050);
    assert(output.used_studio_fm);

    assert(transport.pop_final(output));
    assert(output.destination_ordinal == 12);
    assert(output.left == 3150 && output.right == -3150);
    assert(output.used_studio_fm);

    assert(transport.pop_final(output));
    assert(output.destination_ordinal == 13);
    assert(output.left == 4000 && output.right == -4000);
    assert(!output.used_studio_fm);
    assert(transport.empty());

    // Runtime evidence loss is a quality downgrade, never a timeline rewrite.
    // Already-finalized Studio frames stay valid; unresolved frames become their
    // protected PlayerA reference values and drain in original order.
    transport.reset();
    assert(transport.push(frame(100, 5000, -5000, 500, -500, true)));
    assert(transport.push(frame(101, 6000, -6000, 600, -600, true)));
    assert(transport.apply_studio_fm(100, 700, -700));
    transport.fail_closed_reference();
    assert(transport.waiting_frames() == 0);
    assert(transport.contiguous_final_frames() == 2);

    assert(transport.pop_final(output));
    assert(output.destination_ordinal == 100);
    assert(output.left == 5200 && output.right == -5200);
    assert(output.used_studio_fm);
    assert(transport.pop_final(output));
    assert(output.destination_ordinal == 101);
    assert(output.left == 6000 && output.right == -6000);
    assert(!output.used_studio_fm);

    // EOF fallback is stricter than generic evidence failure. The observer's
    // impossible symmetric-FIR tail count must exactly equal the unresolved
    // whole-frame count or the runtime must treat it as a contract fault.
    transport.reset();
    assert(transport.push(frame(200, 7000, -7000, 700, -700, true)));
    assert(transport.push(frame(201, 8000, -8000, 800, -800, true)));
    assert(!transport.finish_reference_tail(1));
    assert(transport.valid());
    assert(transport.waiting_frames() == 2);
    assert(transport.finish_reference_tail(2));
    assert(transport.waiting_frames() == 0);
    assert(transport.pop_final(output));
    assert(output.destination_ordinal == 200 && !output.used_studio_fm);
    assert(transport.pop_final(output));
    assert(output.destination_ordinal == 201 && !output.used_studio_fm);

    // Replacement arithmetic must never wrap PlayerA's signed 32-bit output.
    // A candidate overflow leaves the frame unresolved so the caller can invoke
    // fail_closed_reference() and deliver the exact protected reference instead.
    transport.reset();
    assert(transport.push(frame(
        300,
        std::numeric_limits<std::int32_t>::max(),
        std::numeric_limits<std::int32_t>::min(),
        0,
        0,
        true)));
    assert(!transport.apply_studio_fm(300, 1, -1));
    assert(transport.valid());
    assert(transport.waiting_frames() == 1);
    transport.fail_closed_reference();
    assert(transport.pop_final(output));
    assert(output.left == std::numeric_limits<std::int32_t>::max());
    assert(output.right == std::numeric_limits<std::int32_t>::min());
    assert(!output.used_studio_fm);

    // Destination ordinals are timeline authority. Gaps, duplicates and the
    // uint64 terminal value invalidate transport instead of being repaired.
    transport.reset();
    assert(transport.push(frame(400, 1, 2, 0, 0, false)));
    assert(!transport.push(frame(402, 3, 4, 0, 0, false)));
    assert(!transport.valid());
    assert(!transport.pop_final(output));

    transport.reset();
    assert(!transport.push(frame(
        std::numeric_limits<std::uint64_t>::max(), 1, 2, 0, 0, false)));
    assert(!transport.valid());

    // Capacity is a fail-closed boundary too. No silent oldest/newest frame drop
    // is permitted because either policy would phase-shift unrelated chips.
    studio_frame_transport<2> bounded;
    bounded.reset();
    assert(bounded.push(frame(0, 10, 20, 1, 2, true)));
    assert(bounded.push(frame(1, 30, 40, 3, 4, true)));
    assert(bounded.full());
    assert(!bounded.push(frame(2, 50, 60, 5, 6, true)));
    assert(!bounded.valid());
    assert(bounded.contiguous_final_frames() == 0);
    assert(!bounded.pop_final(output));

    // Ring wrap must preserve destination ordering indefinitely.
    studio_frame_transport<3> wrapped;
    wrapped.reset();
    for (std::uint64_t ordinal = 0; ordinal < 32; ++ordinal) {
        assert(wrapped.push(frame(
            ordinal,
            static_cast<std::int32_t>(ordinal * 10),
            -static_cast<std::int32_t>(ordinal * 10),
            1,
            -1,
            false)));
        assert(wrapped.pop_final(output));
        assert(output.destination_ordinal == ordinal);
        assert(output.left == static_cast<std::int32_t>(ordinal * 10));
        assert(!output.used_studio_fm);
    }
    assert(wrapped.empty());
    assert(wrapped.valid());

    return 0;
}
