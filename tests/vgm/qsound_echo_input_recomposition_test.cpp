#include "../../components/vgm/enhancement/qsound_echo_input_recomposition.h"

#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    qsound_native_source_frame source;
    qsound_native_mix_frame mix;
    source.native_sample = 100;
    mix.native_sample = 100;
    mix.accounting_valid = true;

    source.source[0] = 100;
    mix.pcm_echo_contribution[0] = 2;
    source.source[1] = -50;
    mix.pcm_echo_contribution[1] = 3;
    mix.echo_input = 200; // 100*2*4 + (-50)*3*4

    auto result = qsound_recompose_echo_input(source, mix);
    CHECK(result.status == qsound_echo_input_status::exact);
    CHECK(result.recomposed == 200);
    CHECK(qsound_echo_input_recomposes_exactly(source, mix));

    mix.echo_input = 199;
    CHECK(!qsound_echo_input_recomposes_exactly(source, mix));
    mix.echo_input = 200;

    mix.accounting_valid = false;
    result = qsound_recompose_echo_input(source, mix);
    CHECK(result.status == qsound_echo_input_status::accounting_unavailable);
    CHECK(!qsound_echo_input_recomposes_exactly(source, mix));
    mix.accounting_valid = true;

    mix.native_sample = 101;
    result = qsound_recompose_echo_input(source, mix);
    CHECK(result.status == qsound_echo_input_status::native_sample_mismatch);
    mix.native_sample = 100;

    // A single scaled contribution can exceed the historical INT32 domain.
    source = {};
    mix = {};
    source.native_sample = 200;
    mix.native_sample = 200;
    mix.accounting_valid = true;
    source.source[0] = 32767;
    mix.pcm_echo_contribution[0] = 32767;
    result = qsound_recompose_echo_input(source, mix);
    CHECK(result.status == qsound_echo_input_status::historical_int32_overflow_domain);

    // The individual terms may fit while the running historical accumulator
    // does not. Preserve that as a separate uncertified arithmetic region.
    source = {};
    mix = {};
    source.native_sample = 300;
    mix.native_sample = 300;
    mix.accounting_valid = true;
    source.source[0] = 20000;
    source.source[1] = 20000;
    mix.pcm_echo_contribution[0] = 20000;
    mix.pcm_echo_contribution[1] = 20000;
    result = qsound_recompose_echo_input(source, mix);
    CHECK(result.status == qsound_echo_input_status::historical_int32_overflow_domain);

    return 0;
}
