#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace gameaudio::spc {

// The restoration seam intentionally operates at a BRR-block boundary, before
// SNESAPU interpolation, envelope, routing, pitch modulation and echo send.
// One BRR block always represents sixteen decoded game-grid samples. A verified
// original/pre-BRR source is first transformed through the proven historical
// preparation map onto this exact game grid; only then can these samples replace
// BRR decompression output.
constexpr std::size_t snes_prebrr_samples_per_block = 16;
constexpr std::uint32_t snes_brr_address_modulus = 65536u;
constexpr std::uint32_t snes_brr_block_bytes = 9u;
// 9 * 36409 == 1 mod 65536. Because gcd(9, 65536) == 1, this maps a wrapped
// block-address delta back to one unambiguous sequential BRR block ordinal.
constexpr std::uint32_t snes_brr_block_inverse_mod_65536 = 36409u;

struct snes_prebrr_sample_entry {
    std::uint8_t source_number = 0;
    std::uint16_t first_brr_block_address = 0;
    std::uint32_t block_count = 0;
    const std::int16_t* prepared_pcm = nullptr;
    std::size_t prepared_frame_count = 0;

    [[nodiscard]] bool valid() const noexcept {
        return block_count != 0
            && prepared_pcm != nullptr
            && prepared_frame_count
                == static_cast<std::size_t>(block_count) * snes_prebrr_samples_per_block;
    }
};

inline std::uint32_t snes_prebrr_block_ordinal(
    std::uint16_t first_brr_block_address,
    std::uint16_t current_brr_block_address) noexcept
{
    const std::uint32_t delta = static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(current_brr_block_address)
        - static_cast<std::uint32_t>(first_brr_block_address));
    return (delta * snes_brr_block_inverse_mod_65536) & 0xffffu;
}

template <std::size_t MaxEntries = 256>
class snesapu_prebrr_provider {
    static_assert(MaxEntries > 0, "MaxEntries must be non-zero");

public:
    void clear() noexcept { count_ = 0; }
    [[nodiscard]] std::size_t count() const noexcept { return count_; }

    bool add(const snes_prebrr_sample_entry& entry) noexcept {
        if (!entry.valid() || count_ >= entries_.size())
            return false;

        // One SRCN is one runtime identity in this provider. Multiple different
        // approved entries for the same source would make playback ambiguous.
        for (std::size_t index = 0; index < count_; ++index) {
            if (entries_[index].source_number == entry.source_number)
                return false;
        }
        entries_[count_++] = entry;
        return true;
    }

    [[nodiscard]] const snes_prebrr_sample_entry* find(
        std::uint8_t source_number) const noexcept
    {
        for (std::size_t index = 0; index < count_; ++index) {
            if (entries_[index].source_number == source_number)
                return &entries_[index];
        }
        return nullptr;
    }

    bool fill_block(
        std::uint8_t source_number,
        std::uint16_t brr_block_address,
        std::int16_t output[snes_prebrr_samples_per_block]) const noexcept
    {
        if (output == nullptr)
            return false;
        const auto* entry = find(source_number);
        if (entry == nullptr || !entry->valid())
            return false;

        const std::uint32_t ordinal = snes_prebrr_block_ordinal(
            entry->first_brr_block_address,
            brr_block_address);
        if (ordinal >= entry->block_count)
            return false;

        // Guard the inverse mapping explicitly. This catches a malformed address
        // that happens to produce an in-range modular ordinal under a corrupted
        // entry before any replacement reaches the synthesis path.
        const std::uint16_t expected = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(entry->first_brr_block_address)
            + ordinal * snes_brr_block_bytes);
        if (expected != brr_block_address)
            return false;

        const std::size_t first_frame = static_cast<std::size_t>(ordinal)
            * snes_prebrr_samples_per_block;
        if (first_frame + snes_prebrr_samples_per_block > entry->prepared_frame_count)
            return false;

        std::memcpy(
            output,
            entry->prepared_pcm + first_frame,
            snes_prebrr_samples_per_block * sizeof(std::int16_t));
        return true;
    }

private:
    std::array<snes_prebrr_sample_entry, MaxEntries> entries_{};
    std::size_t count_ = 0;
};

// C ABI shape expected by the patched 32-bit SNESAPU hot-path boundary. The
// actual callback is installed inside the spcplayer child process so cross-
// process calls never occur. A return value of zero means "use exact BRR".
using snesapu_prebrr_callback = int (*)(
    void* user,
    std::uint32_t source_number,
    std::uint32_t brr_block_address,
    std::int16_t* output16);

} // namespace gameaudio::spc
