#include "../../components/spc/spc_ram_generation.h"

#include <cstddef>
#include <cstdint>

using namespace gameaudio::spc;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    spc_ram_generation_tracker tracker;

    CHECK(tracker.write_serial() == 0);
    CHECK(tracker.page_generation(0x00) == 0);
    CHECK(tracker.page_generation(0xFF) == 0);

    const auto two_page_extent = tracker.capture_extent(0x1200, 0x180);
    CHECK(two_page_extent.pages.size() == 2);
    CHECK(two_page_extent.pages[0].page == 0x12);
    CHECK(two_page_extent.pages[1].page == 0x13);
    CHECK(tracker.extent_unchanged(two_page_extent));

    // An unrelated write advances global write order without invalidating an
    // extent whose pages were untouched.
    tracker.mark_write(0x1400, 1);
    CHECK(tracker.write_serial() == 1);
    CHECK(tracker.page_generation(0x14) == 1);
    CHECK(tracker.page_generation(0x12) == 0);
    CHECK(tracker.extent_unchanged(two_page_extent));

    // A write to either observed page invalidates the captured extent.
    tracker.mark_write(0x12FF, 1);
    CHECK(tracker.write_serial() == 2);
    CHECK(tracker.page_generation(0x12) == 2);
    CHECK(!tracker.extent_unchanged(two_page_extent));

    // One multi-byte write assigns one generation to every page it touches.
    const std::uint64_t before_cross_page = tracker.write_serial();
    tracker.mark_write(0x20F0, 0x30);
    CHECK(tracker.write_serial() == before_cross_page + 1);
    CHECK(tracker.page_generation(0x20) == tracker.write_serial());
    CHECK(tracker.page_generation(0x21) == tracker.write_serial());

    // APURAM addressing is 16-bit. Cross-boundary writes touch page FF and
    // page 00 rather than an invented 17-bit address space.
    const auto wrapped_extent = tracker.capture_extent(0xFFF0, 0x30);
    CHECK(wrapped_extent.pages.size() == 2);
    CHECK(wrapped_extent.pages[0].page == 0xFF);
    CHECK(wrapped_extent.pages[1].page == 0x00);
    CHECK(tracker.extent_unchanged(wrapped_extent));

    tracker.mark_write(0x0001, 1);
    CHECK(!tracker.extent_unchanged(wrapped_extent));

    const std::uint64_t before_wrapped_write = tracker.write_serial();
    tracker.mark_write(0xFFFF, 2);
    CHECK(tracker.write_serial() == before_wrapped_write + 1);
    CHECK(tracker.page_generation(0xFF) == tracker.write_serial());
    CHECK(tracker.page_generation(0x00) == tracker.write_serial());

    // Zero-byte reports are no-ops and cannot create fake memory mutations.
    const std::uint64_t before_zero = tracker.write_serial();
    tracker.mark_write(0x4000, 0);
    CHECK(tracker.write_serial() == before_zero);

    // A whole-RAM restore invalidates every page with one coherent generation.
    const auto page_7_before_reset = tracker.capture_extent(0x0700, 9);
    CHECK(tracker.extent_unchanged(page_7_before_reset));
    tracker.mark_all();
    CHECK(!tracker.extent_unchanged(page_7_before_reset));
    for (std::size_t page = 0; page < spc_ram_page_count; ++page)
        CHECK(tracker.page_generation(static_cast<std::uint8_t>(page)) == tracker.write_serial());

    // Capturing all 64 KiB produces exactly 256 unique page stamps even when
    // the logical extent begins away from page zero and wraps once.
    const auto all_ram = tracker.capture_extent(0x0080, spc_runtime_ram_size);
    CHECK(all_ram.pages.size() == spc_ram_page_count);
    CHECK(tracker.extent_unchanged(all_ram));

    tracker.mark_write(0x8000, 4);
    CHECK(!tracker.extent_unchanged(all_ram));

    // Empty extents intentionally have no memory continuity dependency.
    const auto empty = tracker.capture_extent(0x1234, 0);
    CHECK(empty.pages.empty());
    tracker.mark_all();
    CHECK(tracker.extent_unchanged(empty));

    return 0;
}
