#include "../../components/vgm/enhancement/qsound_native_time_map.h"

#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    qsound_native_time_map map;
    qsound_native_time_point point;

    CHECK(!map.project(0, point));
    CHECK(!map.configure(24038, 22050));
    CHECK(!map.supported());

    CHECK(map.configure(24038, 48000));
    CHECK(map.supported());
    CHECK(map.native_rate() == 24038u);
    CHECK(map.output_rate() == 48000u);

    CHECK(map.project(0, point));
    CHECK(point.native_floor == 0u);
    CHECK(point.fraction_numerator == 0u);
    CHECK(point.fraction_denominator == 48000u);

    // Absolute-time projection: one output second maps to one native second.
    CHECK(map.project(48000, point));
    CHECK(point.native_floor == 24038u);
    CHECK(point.fraction_numerator == 0u);

    // Consecutive output frames share one rational phase map. No lane-local
    // accumulator exists here that could drift independently.
    CHECK(map.project(1, point));
    CHECK(point.native_floor == 0u);
    CHECK(point.fraction_numerator == 24038u);

    CHECK(map.project(2, point));
    CHECK(point.native_floor == 1u);
    CHECK(point.fraction_numerator == 76u);

    CHECK(map.project(47999, point));
    const std::uint64_t numerator = 47999ull * 24038ull;
    CHECK(point.native_floor == numerator / 48000ull);
    CHECK(point.fraction_numerator == numerator % 48000ull);

    // Exact-rate operation has zero interpolation fraction everywhere.
    CHECK(map.configure(24038, 24038));
    CHECK(map.project(1234567, point));
    CHECK(point.native_floor == 1234567u);
    CHECK(point.fraction_numerator == 0u);
    CHECK(point.fraction_denominator == 24038u);

    return 0;
}
