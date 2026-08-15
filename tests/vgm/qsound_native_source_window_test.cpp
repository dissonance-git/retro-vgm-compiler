#include "../../components/vgm/enhancement/qsound_native_source_window.h"

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static qsound_native_source_frame make_frame(std::uint64_t index, std::int16_t base) {
    qsound_native_source_frame frame;
    frame.native_sample = index;
    for (std::size_t lane = 0; lane < qsound_native_source_count; ++lane)
        frame.source[lane] = static_cast<std::int16_t>(base + lane);
    return frame;
}

int main() {
    qsound_native_source_window window;
    qsound_native_source_bracket bracket;

    // The first observed block may legitimately begin after native sample zero.
    // Missing earlier evidence stays unavailable.
    qsound_native_source_frame first[] = {
        make_frame(1, 100),
        make_frame(2, 200),
    };
    CHECK(window.begin_block(first, 2));

    qsound_native_time_point point;
    point.native_floor = 0;
    point.fraction_numerator = 0;
    point.fraction_denominator = 48000;
    CHECK(!window.find_bracket(point, bracket));

    point.native_floor = 1;
    CHECK(window.find_bracket(point, bracket));
    CHECK(bracket.available());
    CHECK(bracket.lower->native_sample == 1u);
    CHECK(bracket.upper->native_sample == 1u);

    point.fraction_numerator = 24000;
    CHECK(window.find_bracket(point, bracket));
    CHECK(bracket.lower->native_sample == 1u);
    CHECK(bracket.upper->native_sample == 2u);
    CHECK(bracket.lower->source[7] == 107);
    CHECK(bracket.upper->source[7] == 207);

    CHECK(window.end_block());
    CHECK(window.history_count() == 2u);

    // Cross-block interpolation can use the retained last native sample and the
    // first sample newly pulled by the next reference decode block.
    qsound_native_source_frame second[] = { make_frame(3, 300) };
    CHECK(window.begin_block(second, 1));
    point.native_floor = 2;
    point.fraction_numerator = 12000;
    CHECK(window.find_bracket(point, bracket));
    CHECK(bracket.lower->native_sample == 2u);
    CHECK(bracket.upper->native_sample == 3u);
    CHECK(window.end_block());

    // Empty destination blocks do not destroy retained source history.
    CHECK(window.begin_block(nullptr, 0));
    point.native_floor = 3;
    point.fraction_numerator = 0;
    CHECK(window.find_bracket(point, bracket));
    CHECK(bracket.lower->native_sample == 3u);
    CHECK(window.end_block());

    // A discontinuity is structural corruption and fails closed.
    qsound_native_source_frame gap[] = { make_frame(5, 500) };
    CHECK(!window.begin_block(gap, 1));
    CHECK(!window.valid());

    window.reset();
    CHECK(window.valid());
    CHECK(window.begin_block(first, 2));
    point.native_floor = 1;
    point.fraction_numerator = 48000;
    point.fraction_denominator = 48000;
    CHECK(!window.find_bracket(point, bracket));

    return 0;
}
