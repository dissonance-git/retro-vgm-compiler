#include "../../components/vgm/enhancement/qsound_native_mix_capture.h"

#include <cstddef>
#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    qsound_native_mix_capture capture;
    qsound_native_mix_frame frame;
    frame.native_sample = 500;
    frame.accounting_valid = true;
    frame.pcm_echo_contribution[0] = 123;
    frame.pcm_echo_contribution[15] = -456;
    frame.echo_input = 123456;
    frame.echo_output = -321;
    frame.wet_post_delay = {{1000, -2000}};
    frame.dry_post_delay = {{3000, 4000}};
    frame.reference_output = {{17, -23}};

    capture.begin_block();
    CHECK(capture.valid());
    CHECK(capture.count() == 0u);
    CHECK(capture.native_sample_rate() == 0u);

    capture.observe(0, 24038, &frame);
    CHECK(capture.valid());
    CHECK(capture.count() == 1u);
    CHECK(capture.native_sample_rate() == 24038u);
    CHECK(capture.first_native_sample() == 500u);
    CHECK(capture.frames()[0].accounting_valid);
    CHECK(capture.frames()[0].pcm_echo_contribution[0] == 123);
    CHECK(capture.frames()[0].pcm_echo_contribution[15] == -456);
    CHECK(capture.frames()[0].echo_input == 123456);
    CHECK(capture.frames()[0].echo_output == -321);
    CHECK(capture.frames()[0].wet_post_delay[0] == 1000);
    CHECK(capture.frames()[0].wet_post_delay[1] == -2000);
    CHECK(capture.frames()[0].dry_post_delay[0] == 3000);
    CHECK(capture.frames()[0].dry_post_delay[1] == 4000);
    CHECK(capture.frames()[0].reference_output[0] == 17);
    CHECK(capture.frames()[0].reference_output[1] == -23);

    // A QSound init/filter-refresh tick is still a real native timeline frame.
    // It is not a structural capture error merely because normal mix accounting
    // was unavailable on that tick.
    frame.native_sample = 501;
    frame.accounting_valid = false;
    frame.pcm_echo_contribution.fill(0);
    frame.echo_input = 0;
    frame.echo_output = 0;
    frame.wet_post_delay = {{0, 0}};
    frame.dry_post_delay = {{0, 0}};
    frame.reference_output = {{0, 0}};
    capture.observe(0, 24038, &frame);
    CHECK(capture.valid());
    CHECK(capture.count() == 2u);
    CHECK(!capture.frames()[1].accounting_valid);
    CHECK(capture.frames()[1].pcm_echo_contribution[0] == 0);
    CHECK(capture.frames()[1].pcm_echo_contribution[15] == 0);

    frame.native_sample = 502;
    frame.accounting_valid = true;
    frame.pcm_echo_contribution[0] = -11;
    frame.echo_output = 77;
    capture.observe(0, 24038, &frame);
    CHECK(capture.valid());
    CHECK(capture.count() == 3u);
    CHECK(capture.frames()[2].accounting_valid);
    CHECK(capture.frames()[2].pcm_echo_contribution[0] == -11);
    CHECK(capture.frames()[2].echo_output == 77);

    // One coherent native timeline is mandatory. Never repair a gap/reorder.
    frame.native_sample = 504;
    capture.observe(0, 24038, &frame);
    CHECK(!capture.valid());
    CHECK(capture.count() == 3u);

    capture.begin_block();
    frame.native_sample = 1;
    capture.observe(0, 24038, &frame);
    frame.native_sample = 2;
    capture.observe(0, 24039, &frame);
    CHECK(!capture.valid());
    CHECK(capture.count() == 1u);

    capture.begin_block();
    frame.native_sample = 1;
    capture.observe(1, 24038, &frame);
    CHECK(!capture.valid());
    CHECK(capture.count() == 0u);

    capture.begin_block();
    capture.observe(0, 0, &frame);
    CHECK(!capture.valid());

    capture.begin_block();
    capture.observe(0, 24038, nullptr);
    CHECK(!capture.valid());

    capture.begin_block();
    for (std::size_t i = 0; i < qsound_native_mix_capture::capacity; ++i) {
        frame.native_sample = static_cast<std::uint64_t>(10000 + i);
        capture.observe(0, 24038, &frame);
        CHECK(capture.valid());
    }
    CHECK(capture.count() == qsound_native_mix_capture::capacity);
    frame.native_sample = static_cast<std::uint64_t>(10000 + qsound_native_mix_capture::capacity);
    capture.observe(0, 24038, &frame);
    CHECK(capture.overflowed());
    CHECK(!capture.valid());
    CHECK(capture.count() == qsound_native_mix_capture::capacity);

    return 0;
}
