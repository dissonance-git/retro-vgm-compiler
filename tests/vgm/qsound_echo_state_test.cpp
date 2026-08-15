#include "../../components/vgm/enhancement/qsound_echo_state.h"

#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    qsound_echo_state echo(qsound_echo_mode::mode1);
    CHECK(echo.mode() == qsound_echo_mode::mode1);
    CHECK(echo.end_position() == 0x055au);
    CHECK(echo.feedback() == 0);
    CHECK(echo.delay_position() == 0u);

    // Default mode-1 echo length is six samples. Store +2.0 in the first
    // Q16-scaled delay entry, traverse the remaining five entries, then observe
    // the recovered two-sample moving average on wrap.
    auto step = echo.step(2 << 16);
    CHECK(step.status == qsound_echo_step_status::exact);
    CHECK(step.output == 0);
    for (int i = 0; i < 5; ++i) {
        step = echo.step(0);
        CHECK(step.status == qsound_echo_step_status::exact);
        CHECK(step.output == 0);
    }
    CHECK(echo.delay_position() == 0u);
    step = echo.step(0);
    CHECK(step.status == qsound_echo_step_status::exact);
    CHECK(step.output == 1);
    CHECK(echo.last_sample() == 2);

    // The next empty delay slot is averaged with the previous raw delay sample.
    step = echo.step(0);
    CHECK(step.status == qsound_echo_step_status::exact);
    CHECK(step.output == 1);
    CHECK(echo.last_sample() == 0);

    // Negative averaging uses explicit arithmetic-right-shift semantics.
    echo.reset(qsound_echo_mode::mode1);
    CHECK(echo.step(-(2 << 16)).output == 0);
    for (int i = 0; i < 5; ++i)
        CHECK(echo.step(0).status == qsound_echo_step_status::exact);
    step = echo.step(0);
    CHECK(step.status == qsound_echo_step_status::exact);
    CHECK(step.output == -1);

    // Length zero is a valid historical state: the single active position wraps
    // to itself every tick.
    echo.reset(qsound_echo_mode::mode1);
    echo.set_end_position(0x0554u);
    CHECK(echo.step(2 << 16).output == 0);
    CHECK(echo.delay_position() == 0u);
    step = echo.step(0);
    CHECK(step.status == qsound_echo_step_status::exact);
    CHECK(step.output == 1);

    // Signed feedback comes directly from the 16-bit register word.
    echo.reset(qsound_echo_mode::mode1);
    echo.set_feedback(0xffffu);
    CHECK(echo.feedback() == -1);

    // Mode 2 has a different end-position base but the recovered initializer
    // still starts at a six-sample echo length.
    echo.reset(qsound_echo_mode::mode2);
    CHECK(echo.mode() == qsound_echo_mode::mode2);
    CHECK(echo.end_position() == 0x0542u);

    // Refuse the pinned C core's implementation-defined INT16 narrowing region
    // instead of pretending an arbitrary raw end position has portable meaning.
    echo.reset(qsound_echo_mode::mode1);
    echo.set_end_position(0xffffu);
    const std::size_t before_bad_end = echo.delay_position();
    step = echo.step(0);
    CHECK(step.status == qsound_echo_step_status::end_position_conversion_domain);
    CHECK(echo.delay_position() == before_bad_end);

    // Build enough positive history to make maximum feedback overflow the
    // historical INT32 feedback term. The failing transition must be atomic.
    echo.reset(qsound_echo_mode::mode1);
    for (int i = 0; i < 6; ++i) {
        step = echo.step(32767 << 16);
        CHECK(step.status == qsound_echo_step_status::exact);
    }
    echo.set_feedback(0x7fffu);
    step = echo.step(0);
    CHECK(step.status == qsound_echo_step_status::exact);
    CHECK(step.output == 16383);
    const std::size_t before_overflow = echo.delay_position();
    const std::int16_t last_before_overflow = echo.last_sample();
    step = echo.step(0);
    CHECK(step.status == qsound_echo_step_status::historical_int32_overflow_domain);
    CHECK(echo.delay_position() == before_overflow);
    CHECK(echo.last_sample() == last_before_overflow);

    // A seed carries only the recurrence memory. Runtime feedback/length can
    // then vary independently on every observed native tick.
    qsound_echo_seed seed;
    seed.last_sample = 6;
    seed.delay_position = 3;
    seed.delay_line[3] = 10;
    echo.reset(qsound_echo_mode::mode1);
    CHECK(echo.load_seed(seed));
    CHECK(echo.last_sample() == 6);
    CHECK(echo.delay_position() == 3u);
    step = echo.step_runtime(0, 0, 6);
    CHECK(step.status == qsound_echo_step_status::exact);
    CHECK(step.output == 8);
    CHECK(echo.last_sample() == 10);
    CHECK(echo.delay_position() == 4u);

    const qsound_echo_seed after = echo.seed();
    CHECK(after.last_sample == 10);
    CHECK(after.delay_position == 4u);
    CHECK(after.delay_line[3] == 0);

    qsound_echo_seed invalid_seed = after;
    invalid_seed.delay_position = qsound_echo_state::delay_capacity;
    const qsound_echo_seed before_invalid_seed = echo.seed();
    CHECK(!echo.load_seed(invalid_seed));
    CHECK(echo.seed().last_sample == before_invalid_seed.last_sample);
    CHECK(echo.seed().delay_position == before_invalid_seed.delay_position);

    const qsound_echo_seed before_bad_runtime = echo.seed();
    step = echo.step_runtime(0, 0, 1025);
    CHECK(step.status == qsound_echo_step_status::runtime_length_out_of_range);
    CHECK(echo.seed().last_sample == before_bad_runtime.last_sample);
    CHECK(echo.seed().delay_position == before_bad_runtime.delay_position);

    return 0;
}
