#include "../../components/vgm/enhancement/qsound_native_linear_projection.h"

#include <array>
#include <cmath>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static bool near(float a, float b, float tolerance = 1.0e-6f) {
    return std::fabs(a - b) <= tolerance;
}

int main() {
    qsound_native_source_frame lower;
    qsound_native_source_frame upper;
    for (std::size_t lane = 0; lane < qsound_native_source_count; ++lane) {
        lower.source[lane] = static_cast<std::int16_t>(lane * 100);
        upper.source[lane] = static_cast<std::int16_t>(lane * 100 + 100);
    }

    qsound_native_source_bracket bracket;
    bracket.lower = &lower;
    bracket.upper = &upper;
    bracket.fraction_numerator = 1;
    bracket.fraction_denominator = 2;

    std::array<float, qsound_native_source_count> out{};
    CHECK(qsound_project_native_linear(bracket, out));
    for (std::size_t lane = 0; lane < qsound_native_source_count; ++lane) {
        const float expected = static_cast<float>(lane * 100 + 50) / 32768.0f;
        CHECK(near(out[lane], expected));
    }

    // Exact native coordinates do not require a second distinct sample.
    bracket.upper = &lower;
    bracket.fraction_numerator = 0;
    bracket.fraction_denominator = 48000;
    CHECK(qsound_project_native_linear(bracket, out));
    CHECK(near(out[7], 700.0f / 32768.0f));

    // Preserve the full signed source range and normalize only at the consumer
    // boundary. -32768 is exactly -1.0; +32767 remains just below +1.0.
    lower.source[0] = -32768;
    lower.source[1] = 32767;
    CHECK(qsound_project_native_linear(bracket, out));
    CHECK(out[0] == -1.0f);
    CHECK(near(out[1], 32767.0f / 32768.0f));

    qsound_native_source_bracket missing;
    out.fill(1.0f);
    CHECK(!qsound_project_native_linear(missing, out));
    for (float sample : out)
        CHECK(sample == 0.0f);

    return 0;
}
