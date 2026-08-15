#include "../../components/vgm/enhancement/qsound_native_source_capture.h"

#include <array>
#include <cstddef>
#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    qsound_native_source_capture capture;
    std::array<std::int16_t, qsound_native_source_count> source{};
    for (std::size_t i = 0; i < source.size(); ++i)
        source[i] = static_cast<std::int16_t>(static_cast<int>(i) * 257 - 2000);

    capture.begin_block();
    CHECK(capture.valid());
    CHECK(capture.count() == 0u);
    CHECK(capture.native_sample_rate() == 0u);

    capture.observe(0, 24038, 9000, source.data(), source.size());
    CHECK(capture.valid());
    CHECK(capture.count() == 1u);
    CHECK(capture.native_sample_rate() == 24038u);
    CHECK(capture.first_native_sample() == 9000u);
    CHECK(capture.frames()[0].native_sample == 9000u);
    for (std::size_t i = 0; i < source.size(); ++i)
        CHECK(capture.frames()[0].source[i] == source[i]);

    source[0] = 1234;
    source[18] = -2345;
    capture.observe(0, 24038, 9001, source.data(), source.size());
    CHECK(capture.valid());
    CHECK(capture.count() == 2u);
    CHECK(capture.frames()[1].source[0] == 1234);
    CHECK(capture.frames()[1].source[18] == -2345);

    // Native QSound frames form one ordered timeline. A gap or reorder is not
    // repaired because later shared-phase conversion depends on exact spacing.
    capture.observe(0, 24038, 9003, source.data(), source.size());
    CHECK(!capture.valid());
    CHECK(capture.count() == 2u);

    capture.begin_block();
    capture.observe(0, 24038, 10, source.data(), source.size());
    capture.observe(0, 24039, 11, source.data(), source.size());
    CHECK(!capture.valid());
    CHECK(capture.count() == 1u);

    capture.begin_block();
    capture.observe(1, 24038, 10, source.data(), source.size());
    CHECK(!capture.valid());
    CHECK(capture.count() == 0u);

    capture.begin_block();
    capture.observe(0, 0, 10, source.data(), source.size());
    CHECK(!capture.valid());

    capture.begin_block();
    capture.observe(0, 24038, 10, nullptr, source.size());
    CHECK(!capture.valid());

    capture.begin_block();
    capture.observe(0, 24038, 10, source.data(), source.size() - 1);
    CHECK(!capture.valid());

    capture.begin_block();
    for (std::size_t i = 0; i < qsound_native_source_capture::capacity; ++i) {
        capture.observe(
            0,
            24038,
            static_cast<std::uint64_t>(100000 + i),
            source.data(),
            source.size());
        CHECK(capture.valid());
    }
    CHECK(capture.count() == qsound_native_source_capture::capacity);
    capture.observe(
        0,
        24038,
        static_cast<std::uint64_t>(100000 + qsound_native_source_capture::capacity),
        source.data(),
        source.size());
    CHECK(capture.overflowed());
    CHECK(!capture.valid());
    CHECK(capture.count() == qsound_native_source_capture::capacity);

    return 0;
}
