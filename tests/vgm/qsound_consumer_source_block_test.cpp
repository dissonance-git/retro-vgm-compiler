#include "../../components/vgm/enhancement/qsound_consumer_source_block.h"

#include <array>
#include <cmath>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static bool near(float a, float b, float tolerance = 1.0e-6f) {
    return std::fabs(a - b) <= tolerance;
}

static qsound_native_source_frame make_frame(std::uint64_t index, std::int16_t base) {
    qsound_native_source_frame frame;
    frame.native_sample = index;
    for (std::size_t lane = 0; lane < qsound_native_source_count; ++lane)
        frame.source[lane] = static_cast<std::int16_t>(base + lane * 10);
    return frame;
}

int main() {
    qsound_native_time_map map;
    CHECK(map.configure(2, 4));

    qsound_native_source_window window;
    qsound_native_source_frame native[] = {
        make_frame(0, 0),
        make_frame(1, 1000),
        make_frame(2, 2000),
    };
    CHECK(window.begin_block(native, 3));

    std::array<std::array<float, 4>, qsound_native_source_count> lanes{};
    std::array<std::uint8_t, 4> available{};
    qsound_consumer_source_target target;
    for (std::size_t lane = 0; lane < qsound_native_source_count; ++lane)
        target.lane[lane] = lanes[lane].data();
    target.availability = available.data();
    target.capacity = available.size();

    const qsound_consumer_source_result result =
        qsound_render_consumer_source_block(map, window, 0, 4, target);
    CHECK(result.structurally_valid);
    CHECK(result.available_frames == 4u);
    CHECK(result.unavailable_frames == 0u);
    for (std::uint8_t flag : available)
        CHECK(flag == 1u);

    CHECK(near(lanes[0][0], 0.0f));
    CHECK(near(lanes[0][1], 500.0f / 32768.0f));
    CHECK(near(lanes[0][2], 1000.0f / 32768.0f));
    CHECK(near(lanes[0][3], 1500.0f / 32768.0f));
    CHECK(near(lanes[7][1], 570.0f / 32768.0f));

    CHECK(window.end_block());

    // Missing startup evidence remains visible and is zero-filled only in the
    // PCM buffer while the availability lane says the frame is not evidence.
    window.reset();
    qsound_native_source_frame late[] = {
        make_frame(1, 1000),
        make_frame(2, 2000),
    };
    CHECK(window.begin_block(late, 2));
    for (auto& lane : lanes)
        lane.fill(1.0f);
    available.fill(1u);

    const qsound_consumer_source_result late_result =
        qsound_render_consumer_source_block(map, window, 0, 4, target);
    CHECK(late_result.structurally_valid);
    CHECK(late_result.available_frames == 2u);
    CHECK(late_result.unavailable_frames == 2u);
    CHECK(available[0] == 0u);
    CHECK(available[1] == 0u);
    CHECK(available[2] == 1u);
    CHECK(available[3] == 1u);
    CHECK(lanes[0][0] == 0.0f);
    CHECK(lanes[0][1] == 0.0f);
    CHECK(near(lanes[0][2], 1000.0f / 32768.0f));

    qsound_consumer_source_target malformed = target;
    malformed.lane[3] = nullptr;
    CHECK(!qsound_render_consumer_source_block(map, window, 0, 1, malformed).structurally_valid);

    qsound_native_time_map downsample;
    CHECK(!downsample.configure(4, 2));
    CHECK(!qsound_render_consumer_source_block(downsample, window, 0, 1, target).structurally_valid);

    return 0;
}
