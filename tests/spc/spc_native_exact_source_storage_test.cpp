#include "components/spc/spc_native_exact_source_storage.h"

#include <cstdint>

using namespace gameaudio::spc;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    {
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t a[spc_native_voice_count] = {
            -32768, -16384, -8192, -1, 0, 8192, 16384, 32767,
        };
        std::int16_t b[spc_native_voice_count] = {
            -16384, -8192, -4096, 0, 1, 4096, 8192, 16384,
        };
        capture.observe(spc_native_sample_rate, 700, a, spc_native_voice_count);
        capture.observe(spc_native_sample_rate, 701, b, spc_native_voice_count);
        CHECK(capture.valid());

        spc_native_exact_source_storage<8> storage;
        CHECK(storage.build(capture, 32000, 1000, 700, 2));
        CHECK(storage.valid());
        CHECK(storage.frame_count() == 2);
        CHECK(storage.reference_start() == 1000);
        CHECK(storage.native_start() == 700);
        CHECK(storage.availability()[0] == 1u);
        CHECK(storage.availability()[1] == 1u);
        CHECK(storage.lane(0)[0] == -1.0f);
        CHECK(storage.lane(1)[0] == -0.5f);
        CHECK(storage.lane(4)[0] == 0.0f);
        CHECK(storage.lane(6)[0] == 0.5f);
        CHECK(storage.lane(7)[0] == 32767.0f / 32768.0f);
        CHECK(storage.lane(7)[1] == 0.5f);
    }

    {
        // A non-native host rate has FIR phase/history in libgme. Refuse to
        // approximate it with a simple rational timeline.
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(spc_native_sample_rate, 0, voices, spc_native_voice_count);
        spc_native_exact_source_storage<4> storage;
        CHECK(!storage.build(capture, 48000, 0, 0, 1));
        CHECK(storage.last_error() ==
            spc_native_exact_source_error::unsupported_output_rate);
        CHECK(!storage.valid());
    }

    {
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(spc_native_sample_rate, 50, voices, spc_native_voice_count);
        spc_native_exact_source_storage<4> storage;
        CHECK(!storage.build(capture, 32000, 10, 49, 1));
        CHECK(storage.last_error() ==
            spc_native_exact_source_error::native_timeline_mismatch);
    }

    {
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(spc_native_sample_rate, 0, voices, spc_native_voice_count);
        spc_native_exact_source_storage<4> storage;
        CHECK(!storage.build(capture, 32000, 0, 0, 2));
        CHECK(storage.last_error() ==
            spc_native_exact_source_error::frame_count_mismatch);
    }

    {
        // Capture invalidity propagates instead of turning the source window into
        // a partially available block.
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(44100, 0, voices, spc_native_voice_count);
        spc_native_exact_source_storage<4> storage;
        CHECK(!storage.build(capture, 32000, 0, 0, 0));
        CHECK(storage.last_error() ==
            spc_native_exact_source_error::invalid_capture);
    }

    return 0;
}
