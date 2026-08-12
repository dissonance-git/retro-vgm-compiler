#pragma once

#include "spc_ram_generation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace gameaudio::spc {

class spc_ram_shadow {
public:
    bool initialized() const noexcept { return initialized_; }
    std::uint64_t synchronized_write_serial() const noexcept { return synchronized_write_serial_; }

    const std::array<std::uint8_t, spc_runtime_ram_size>& bytes() const noexcept {
        return bytes_;
    }

    const std::uint8_t* data() const noexcept { return bytes_.data(); }

    std::uint8_t byte(std::uint16_t address) const noexcept {
        return bytes_[address];
    }

    std::uint64_t page_generation(std::uint8_t page) const noexcept {
        return page_generation_[page];
    }

    // Synchronize only pages whose generation differs. Call at a safe boundary
    // where APURAM is not concurrently being mutated. The first synchronization
    // copies all 64 KiB because generation zero is a legitimate initial state.
    std::size_t synchronize(
        const std::uint8_t* live_ram,
        const spc_ram_generation_tracker& tracker) noexcept {
        if (live_ram == nullptr)
            return 0;

        std::size_t copied_pages = 0;
        for (std::size_t page = 0; page < spc_ram_page_count; ++page) {
            const std::uint8_t page_index = static_cast<std::uint8_t>(page);
            const std::uint64_t generation = tracker.page_generation(page_index);
            if (initialized_ && page_generation_[page] == generation)
                continue;

            const std::size_t offset = page * spc_ram_page_size;
            std::memcpy(bytes_.data() + offset, live_ram + offset, spc_ram_page_size);
            page_generation_[page] = generation;
            ++copied_pages;
        }

        initialized_ = true;
        synchronized_write_serial_ = tracker.write_serial();
        return copied_pages;
    }

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

    // If the shadow was synchronized at or after an event's write serial and no
    // page in the requested extent has a last-write serial newer than that event,
    // then the shadow bytes for that extent are still the exact bytes visible at
    // the event boundary. This is conservative: a write that changes bytes and
    // later restores them still invalidates event-time continuity.
    bool extent_represents_event_time(
        const spc_ram_extent_stamp& stamp,
        std::uint64_t event_write_serial) const noexcept {
        if (!initialized_ || synchronized_write_serial_ < event_write_serial)
            return false;

        for (const auto& page : stamp.pages) {
            if (page_generation_[page.page] > event_write_serial)
                return false;
        }
        return true;
    }

private:
    bool initialized_ = false;
    std::uint64_t synchronized_write_serial_ = 0;
    std::array<std::uint8_t, spc_runtime_ram_size> bytes_{};
    std::array<std::uint64_t, spc_ram_page_count> page_generation_{};
};

} // namespace gameaudio::spc
