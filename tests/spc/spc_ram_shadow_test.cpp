#include "../../components/spc/spc_ram_shadow.h"

#include <array>
#include <cstddef>
#include <cstdint>

using namespace gameaudio::spc;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    std::array<std::uint8_t, spc_runtime_ram_size> live{};
    for (std::size_t i = 0; i < live.size(); ++i)
        live[i] = static_cast<std::uint8_t>(i & 0xFFu);

    spc_ram_generation_tracker tracker;
    spc_ram_shadow shadow;

    CHECK(!shadow.initialized());
    CHECK(shadow.synchronize(live.data(), tracker) == spc_ram_page_count);
    CHECK(shadow.initialized());
    CHECK(shadow.synchronized_write_serial() == 0);
    CHECK(shadow.byte(0x1234) == 0x34);

    // With no writes, a second synchronization copies nothing.
    CHECK(shadow.synchronize(live.data(), tracker) == 0);

    const auto stable = shadow.capture_extent(0x1200, 0x180);
    CHECK(stable.pages.size() == 2);
    CHECK(shadow.extent_unchanged(stable));
    CHECK(shadow.extent_represents_event_time(stable, 0));

    // A write to another page copies only that page and does not invalidate the
    // event-time bytes for an unrelated extent.
    const std::uint64_t unrelated_event_serial = tracker.write_serial();
    live[0x4000] = 0xA5;
    tracker.mark_write(0x4000, 1);
    CHECK(shadow.synchronize(live.data(), tracker) == 1);
    CHECK(shadow.byte(0x4000) == 0xA5);
    CHECK(shadow.extent_unchanged(stable));
    CHECK(shadow.extent_represents_event_time(stable, unrelated_event_serial));

    // Capture the event serial before a relevant write. Once the shadow sees a
    // newer generation on that page it must reject reconstruction of those old
    // event-time bytes, even if the current RAM is otherwise readable.
    const std::uint64_t event_before_relevant_write = tracker.write_serial();
    live[0x12F0] = 0xCC;
    tracker.mark_write(0x12F0, 1);
    CHECK(shadow.synchronize(live.data(), tracker) == 1);
    const auto changed_extent = shadow.capture_extent(0x1200, 0x180);
    CHECK(!shadow.extent_unchanged(stable));
    CHECK(!shadow.extent_represents_event_time(changed_extent, event_before_relevant_write));
    CHECK(shadow.extent_represents_event_time(changed_extent, tracker.write_serial()));

    // A shadow older than the event cannot claim to represent future event-time
    // memory, even if the requested page itself has not changed yet.
    const std::uint64_t future_serial = tracker.write_serial() + 1;
    const auto page_50 = shadow.capture_extent(0x5000, 9);
    CHECK(!shadow.extent_represents_event_time(page_50, future_serial));

    live[0x5000] = 0x44;
    tracker.mark_write(0x5000, 1);
    CHECK(tracker.write_serial() == future_serial);
    CHECK(shadow.synchronize(live.data(), tracker) == 1);
    const auto page_50_after = shadow.capture_extent(0x5000, 9);
    CHECK(shadow.extent_represents_event_time(page_50_after, future_serial));

    // Cross-boundary extents carry both FF and 00 page generations. A later
    // mutation to either side invalidates the earlier event-time view.
    const auto wrapped = shadow.capture_extent(0xFFF8, 0x20);
    CHECK(wrapped.pages.size() == 2);
    const std::uint64_t wrapped_event_serial = tracker.write_serial();
    CHECK(shadow.extent_represents_event_time(wrapped, wrapped_event_serial));

    live[0x0002] = 0x77;
    tracker.mark_write(0x0002, 1);
    CHECK(shadow.synchronize(live.data(), tracker) == 1);
    const auto wrapped_after = shadow.capture_extent(0xFFF8, 0x20);
    CHECK(!shadow.extent_represents_event_time(wrapped_after, wrapped_event_serial));

    // Bulk restore marks all pages and the shadow copies all pages exactly once.
    const std::uint64_t before_restore = tracker.write_serial();
    live.fill(0x5A);
    tracker.mark_all();
    CHECK(tracker.write_serial() == before_restore + 1);
    CHECK(shadow.synchronize(live.data(), tracker) == spc_ram_page_count);
    CHECK(shadow.byte(0x0000) == 0x5A);
    CHECK(shadow.byte(0xFFFF) == 0x5A);

    return 0;
}
