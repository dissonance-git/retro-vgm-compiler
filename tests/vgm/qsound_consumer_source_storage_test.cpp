#include "../../components/vgm/enhancement/qsound_consumer_source_storage.h"

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static qsound_native_source_frame make_frame(std::uint64_t index, std::int16_t value) {
    qsound_native_source_frame frame;
    frame.native_sample = index;
    frame.source.fill(value);
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

    qsound_consumer_source_storage storage;
    const qsound_consumer_source_result result = storage.render(map, window, 0, 4);
    CHECK(result.structurally_valid);
    CHECK(storage.valid());
    CHECK(storage.all_available());
    CHECK(storage.frame_count() == 4u);
    CHECK(storage.lane(0) != nullptr);
    CHECK(storage.lane(qsound_native_source_count) == nullptr);
    CHECK(storage.availability()[0] == 1u);
    CHECK(storage.availability()[3] == 1u);

    storage.reset();
    CHECK(!storage.valid());
    CHECK(!storage.all_available());
    CHECK(storage.frame_count() == 0u);

    qsound_consumer_source_result oversized = storage.render(
        map,
        window,
        0,
        qsound_consumer_source_storage::capacity + 1);
    CHECK(!oversized.structurally_valid);
    CHECK(!storage.valid());

    return 0;
}
