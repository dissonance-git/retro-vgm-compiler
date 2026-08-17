#include "components/spc/spc_native_source_capture.h"

#include <cstddef>
#include <cstdint>

using namespace gameaudio::spc;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    {
        spc_native_source_capture capture;
        capture.reset_trace();

        std::int16_t voices0[spc_native_voice_count] = {
            -32768, -12000, -1, 0, 1, 12000, 30000, 32767,
        };
        std::int16_t voices1[spc_native_voice_count] = {
            -30000, -10000, -2, 0, 2, 10000, 28000, 32000,
        };

        capture.observe(spc_native_sample_rate, 40, voices0, spc_native_voice_count);
        capture.observe(spc_native_sample_rate, 41, voices1, spc_native_voice_count);

        CHECK(capture.valid());
        CHECK(!capture.overflowed());
        CHECK(capture.native_sample_rate() == 32000);
        CHECK(capture.count() == 2);
        CHECK(capture.first_native_sample() == 40);
        CHECK(capture.expected_native_sample_valid());
        CHECK(capture.expected_native_sample() == 42);
        CHECK(capture.frames()[0].native_sample == 40);
        CHECK(capture.frames()[0].source[0] == -32768);
        CHECK(capture.frames()[0].source[7] == 32767);
        CHECK(capture.frames()[1].source[3] == 0);
        CHECK(capture.frames()[1].source[6] == 28000);
    }

    {
        // begin_block drains storage but deliberately preserves the native trace
        // cursor so two protected render calls cannot masquerade as contiguous
        // if a DSP source frame vanished between them.
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(spc_native_sample_rate, 100, voices, spc_native_voice_count);
        CHECK(capture.valid());
        capture.begin_block();
        CHECK(capture.count() == 0);
        CHECK(capture.expected_native_sample() == 101);
        capture.observe(spc_native_sample_rate, 101, voices, spc_native_voice_count);
        CHECK(capture.valid());
        CHECK(capture.count() == 1);
        CHECK(capture.first_native_sample() == 101);
    }

    {
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(spc_native_sample_rate, 7, voices, spc_native_voice_count);
        capture.begin_block();
        capture.observe(spc_native_sample_rate, 9, voices, spc_native_voice_count);
        CHECK(!capture.valid());
        CHECK(!capture.overflowed());
        CHECK(capture.count() == 0);
    }

    {
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(44100, 0, voices, spc_native_voice_count);
        CHECK(!capture.valid());
        CHECK(capture.count() == 0);
    }

    {
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(spc_native_sample_rate, 0, voices, spc_native_voice_count - 1);
        CHECK(!capture.valid());
        CHECK(capture.count() == 0);
    }

    {
        // reset_trace is reserved for a new controlled execution such as seek
        // or track restart. It is the only operation that intentionally permits
        // the native ordinal to restart.
        spc_native_source_capture capture;
        capture.reset_trace();
        std::int16_t voices[spc_native_voice_count] = {};
        capture.observe(spc_native_sample_rate, 500, voices, spc_native_voice_count);
        CHECK(capture.valid());
        capture.reset_trace();
        capture.observe(spc_native_sample_rate, 0, voices, spc_native_voice_count);
        CHECK(capture.valid());
        CHECK(capture.first_native_sample() == 0);
        CHECK(capture.expected_native_sample() == 1);
    }

    return 0;
}
