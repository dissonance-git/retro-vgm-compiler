#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gameaudio::spc {

constexpr std::size_t spc_ram_page_size = 0x100;
constexpr std::size_t spc_ram_page_count = 0x100;
constexpr std::size_t spc_runtime_ram_size = spc_ram_page_size * spc_ram_page_count;

struct spc_ram_page_stamp {
    std::uint8_t page = 0;
    std::uint64_t generation = 0;
};

struct spc_ram_extent_stamp {
    std::uint16_t start_address = 0;
    std::size_t byte_count = 0;
    std::vector<spc_ram_page_stamp> pages;
};

class spc_ram_generation_tracker {
public:
    std::uint64_t write_serial() const noexcept { return write_serial_; }

    std::uint64_t page_generation(std::uint8_t page) const noexcept {
        return page_generation_[page];
    }

    const std::array<std::uint64_t, spc_ram_page_count>& pages() const noexcept {
        return page_generation_;
    }

    // Realtime-safe mutation primitive. No allocation, no locks. One source
    // write obtains one monotonically increasing serial that is assigned to
    // every 256-byte page touched by that write. Addresses wrap exactly as the
    // SPC700's 16-bit APURAM address space does.
    void mark_write(std::uint16_t address, std::size_t byte_count = 1) noexcept {
        if (byte_count == 0)
            return;

        ++write_serial_;
        if (byte_count >= spc_runtime_ram_size) {
            page_generation_.fill(write_serial_);
            return;
        }

        const std::size_t first_page = static_cast<std::size_t>(address >> 8u);
        const std::size_t first_offset = static_cast<std::size_t>(address & 0xFFu);
        const std::size_t pages_touched =
            (first_offset + byte_count + spc_ram_page_size - 1u) / spc_ram_page_size;

        for (std::size_t page_offset = 0; page_offset < pages_touched; ++page_offset) {
            const std::size_t page = (first_page + page_offset) & 0xFFu;
            page_generation_[page] = write_serial_;
        }
    }

    // Use for snapshot load, reset, bulk restore or any path where every APURAM
    // byte may have changed at once.
    void mark_all() noexcept {
        ++write_serial_;
        page_generation_.fill(write_serial_);
    }

    // Analysis-side helper. Capturing an extent stamp may allocate, so it is
    // intentionally separate from the realtime write primitive.
    spc_ram_extent_stamp capture_extent(
        std::uint16_t start_address,
        std::size_t byte_count) const {
        spc_ram_extent_stamp stamp;
        stamp.start_address = start_address;
        stamp.byte_count = byte_count;

        if (byte_count == 0)
            return stamp;

        std::array<bool, spc_ram_page_count> included{};
        const std::size_t bounded =
            byte_count >= spc_runtime_ram_size ? spc_runtime_ram_size : byte_count;
        const std::size_t first_page = static_cast<std::size_t>(start_address >> 8u);
        const std::size_t first_offset = static_cast<std::size_t>(start_address & 0xFFu);
        const std::size_t pages_touched =
            (first_offset + bounded + spc_ram_page_size - 1u) / spc_ram_page_size;

        stamp.pages.reserve(pages_touched > spc_ram_page_count ? spc_ram_page_count : pages_touched);
        for (std::size_t page_offset = 0; page_offset < pages_touched; ++page_offset) {
            const std::uint8_t page = static_cast<std::uint8_t>((first_page + page_offset) & 0xFFu);
            if (included[page])
                continue;
            included[page] = true;
            stamp.pages.push_back({page, page_generation_[page]});
        }

        return stamp;
    }

    bool extent_unchanged(const spc_ram_extent_stamp& stamp) const noexcept {
        for (const auto& page : stamp.pages) {
            if (page_generation_[page.page] != page.generation)
                return false;
        }
        return true;
    }

private:
    std::uint64_t write_serial_ = 0;
    std::array<std::uint64_t, spc_ram_page_count> page_generation_{};
};

} // namespace gameaudio::spc
