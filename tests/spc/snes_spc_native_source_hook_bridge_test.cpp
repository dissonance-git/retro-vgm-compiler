#include "components/spc/snes_spc_native_source_hook_bridge.h"

#include <cstddef>
#include <cstdint>
#include <limits>

using namespace gameaudio::spc;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    {
        spc_native_source_capture capture;
        snes_spc_native_source_hook_bridge hook;
        hook.reset(&capture, 100);
        CHECK(hook.active());
        CHECK(hook.next_native_sample() == 100);
        CHECK(hook.begin_block());

        std::int16_t frame0[spc_native_voice_count] = {
            -100, -50, -1, 0, 1, 50, 100, 150,
        };
        std::int16_t frame1[spc_native_voice_count] = {
            -90, -40, -2, 0, 2, 40, 90, 140,
        };

        CHECK(hook.observe_frame(frame0));
        CHECK(hook.observe_frame(frame1));
        CHECK(capture.valid());
        CHECK(capture.count() == 2);
        CHECK(capture.frames()[0].native_sample == 100);
        CHECK(capture.frames()[1].native_sample == 101);
        CHECK(capture.frames()[0].source[7] == 150);
        CHECK(hook.next_native_sample() == 102);

        // A new protected decoder block drains storage but must not restart the
        // native DSP sample ordinal.
        CHECK(hook.begin_block());
        CHECK(capture.count() == 0);
        CHECK(hook.observe_frame(frame0));
        CHECK(capture.count() == 1);
        CHECK(capture.frames()[0].native_sample == 102);
        CHECK(hook.next_native_sample() == 103);
    }

    {
        // Reset is the explicit discontinuity boundary and may establish an
        // arbitrary new native ordinal after seek/track restart.
        spc_native_source_capture capture;
        snes_spc_native_source_hook_bridge hook;
        std::int16_t frame[spc_native_voice_count] = {};

        hook.reset(&capture, 500);
        CHECK(hook.begin_block());
        CHECK(hook.observe_frame(frame));
        CHECK(capture.first_native_sample() == 500);

        hook.reset(&capture, 7);
        CHECK(hook.begin_block());
        CHECK(hook.observe_frame(frame));
        CHECK(capture.valid());
        CHECK(capture.first_native_sample() == 7);
        CHECK(hook.next_native_sample() == 8);
    }

    {
        // Invalid producer shape must fail closed. The underlying capture is
        // invalidated and the hook deactivates so a missing synthesis instant
        // cannot be silently replaced by a later frame.
        spc_native_source_capture capture;
        snes_spc_native_source_hook_bridge hook;
        std::int16_t frame[spc_native_voice_count] = {};

        hook.reset(&capture, 0);
        CHECK(hook.begin_block());
        CHECK(!hook.observe_frame(frame, spc_native_voice_count - 1));
        CHECK(!capture.valid());
        CHECK(!hook.active());
        CHECK(hook.last_error() == snes_spc_native_source_hook_error::capture_rejected);
        CHECK(!hook.observe_frame(frame));
        CHECK(hook.last_error() == snes_spc_native_source_hook_error::inactive);
    }

    {
        // The uint64 terminal ordinal is deliberately not permitted to wrap.
        spc_native_source_capture capture;
        snes_spc_native_source_hook_bridge hook;
        std::int16_t frame[spc_native_voice_count] = {};

        hook.reset(&capture, std::numeric_limits<std::uint64_t>::max());
        CHECK(hook.begin_block());
        CHECK(!hook.observe_frame(frame));
        CHECK(!capture.valid());
        CHECK(!hook.active());
        CHECK(hook.last_error() == snes_spc_native_source_hook_error::capture_rejected);
    }

    {
        snes_spc_native_source_hook_bridge hook;
        CHECK(!hook.begin_block());
        CHECK(hook.last_error() == snes_spc_native_source_hook_error::inactive);
        hook.deactivate();
        CHECK(!hook.active());
    }

    return 0;
}
