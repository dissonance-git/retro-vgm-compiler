#pragma once

#include "snesapu_studio_source_provider.h"
#include "spc_snapshot.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace gameaudio::spc {

constexpr std::uint32_t snes_studio_source_packet_magic = 0x43525353u; // "SSRC" LE
constexpr std::uint16_t snes_studio_source_packet_version = 1u;
constexpr std::size_t snes_studio_source_packet_header_size = 16u;
constexpr std::size_t snes_studio_source_packet_entry_size = 120u;
constexpr std::uint8_t snes_studio_source_flag_loop = 0x01u;
constexpr std::uint32_t snes_studio_source_no_loop = 0xffffffffu;
constexpr std::size_t snes_studio_source_max_entries = 256u;

static_assert(sizeof(float) == 4, "studio source packet requires binary32 float");
static_assert(sizeof(double) == 8, "studio source packet requires binary64 double");
static_assert(std::numeric_limits<float>::is_iec559,
    "studio source packet requires IEEE-754 float");
static_assert(std::numeric_limits<double>::is_iec559,
    "studio source packet requires IEEE-754 double");

inline std::uint16_t snes_studio_read_le16(const std::uint8_t* data) noexcept {
    return static_cast<std::uint16_t>(data[0])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) << 8u);
}

inline std::uint32_t snes_studio_read_le32(const std::uint8_t* data) noexcept {
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8u)
        | (static_cast<std::uint32_t>(data[2]) << 16u)
        | (static_cast<std::uint32_t>(data[3]) << 24u);
}

inline std::uint64_t snes_studio_read_le64(const std::uint8_t* data) noexcept {
    const std::uint64_t low = snes_studio_read_le32(data);
    const std::uint64_t high = snes_studio_read_le32(data + 4);
    return low | (high << 32u);
}

inline void snes_studio_write_le16(std::uint8_t* data, std::uint16_t value) noexcept {
    data[0] = static_cast<std::uint8_t>(value);
    data[1] = static_cast<std::uint8_t>(value >> 8u);
}

inline void snes_studio_write_le32(std::uint8_t* data, std::uint32_t value) noexcept {
    data[0] = static_cast<std::uint8_t>(value);
    data[1] = static_cast<std::uint8_t>(value >> 8u);
    data[2] = static_cast<std::uint8_t>(value >> 16u);
    data[3] = static_cast<std::uint8_t>(value >> 24u);
}

inline void snes_studio_write_le64(std::uint8_t* data, std::uint64_t value) noexcept {
    snes_studio_write_le32(data, static_cast<std::uint32_t>(value));
    snes_studio_write_le32(data + 4, static_cast<std::uint32_t>(value >> 32u));
}

inline double snes_studio_read_f64(const std::uint8_t* data) noexcept {
    const std::uint64_t bits = snes_studio_read_le64(data);
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

inline void snes_studio_write_f64(std::uint8_t* data, double value) noexcept {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    snes_studio_write_le64(data, bits);
}

inline float snes_studio_read_f32(const std::uint8_t* data) noexcept {
    const std::uint32_t bits = snes_studio_read_le32(data);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

inline void snes_studio_write_f32(std::uint8_t* data, float value) noexcept {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    snes_studio_write_le32(data, bits);
}

constexpr std::size_t snes_studio_align4(std::size_t value) noexcept {
    return (value + 3u) & ~std::size_t{3u};
}

struct snes_studio_source_packet_entry_view {
    std::uint8_t source_number = 0;
    std::uint16_t first_brr_block_address = 0;
    bool loop_present = false;
    std::uint32_t brr_block_count = 0;
    std::uint32_t loop_block_ordinal = snes_studio_source_no_loop;
    std::uint32_t pcm_frame_count = 0;
    std::uint32_t brr_offset_bytes = 0;
    std::uint32_t pcm_offset_bytes = 0;
    spc_sample_content_identity game_brr_identity{};
    spc_sample_content_identity upstream_identity{};
    double game_origin = 0.0;
    double upstream_origin = 0.0;
    double upstream_frames_per_game_sample = 0.0;
    double upstream_loop_start = 0.0;
    double sample_rate_hz = 0.0;
    double game_pcm_units_per_source_unit = 1.0;

    [[nodiscard]] std::uint32_t game_end_sample() const noexcept {
        return brr_block_count * snesapu_brr_samples_per_block;
    }

    [[nodiscard]] std::uint32_t game_loop_start_sample() const noexcept {
        return loop_present
            ? loop_block_ordinal * snesapu_brr_samples_per_block
            : snes_studio_source_no_loop;
    }

    [[nodiscard]] std::uint16_t loop_brr_block_address() const noexcept {
        if (!loop_present)
            return 0u;
        return static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(first_brr_block_address)
            + loop_block_ordinal * snesapu_brr_bytes_per_block);
    }
};

class snes_studio_source_packet_view {
public:
    bool reset(const std::uint8_t* data, std::size_t size) noexcept {
        clear();
        if (data == nullptr || size < snes_studio_source_packet_header_size)
            return false;
        if (snes_studio_read_le32(data) != snes_studio_source_packet_magic
            || snes_studio_read_le16(data + 4) != snes_studio_source_packet_version
            || snes_studio_read_le16(data + 6) != snes_studio_source_packet_header_size)
            return false;

        const std::uint16_t entry_count = snes_studio_read_le16(data + 8);
        const std::uint16_t reserved = snes_studio_read_le16(data + 10);
        const std::uint32_t declared_size = snes_studio_read_le32(data + 12);
        if (entry_count == 0 || entry_count > snes_studio_source_max_entries
            || reserved != 0 || declared_size != size)
            return false;

        const std::uint64_t table_bytes64 = static_cast<std::uint64_t>(entry_count)
            * snes_studio_source_packet_entry_size;
        if (table_bytes64 > size - snes_studio_source_packet_header_size)
            return false;
        std::size_t expected_payload = snes_studio_source_packet_header_size
            + static_cast<std::size_t>(table_bytes64);

        struct source_key {
            std::uint8_t source_number = 0;
            std::uint16_t first_brr_block_address = 0;
        };
        std::array<source_key, snes_studio_source_max_entries> seen{};
        std::size_t seen_count = 0;

        for (std::size_t index = 0; index < entry_count; ++index) {
            const std::uint8_t* raw = entry_bytes(data, index);
            const std::uint8_t flags = raw[1];
            const auto item = entry_unchecked(data, index);
            if ((flags & ~snes_studio_source_flag_loop) != 0u
                || snes_studio_read_le64(raw + 104) != 0u
                || snes_studio_read_le64(raw + 112) != 0u
                || item.brr_block_count == 0
                || item.brr_block_count > spc_ram_size / snesapu_brr_bytes_per_block
                || item.pcm_frame_count == 0
                || !item.game_brr_identity.present()
                || !item.upstream_identity.present()
                || !std::isfinite(item.game_origin)
                || !std::isfinite(item.upstream_origin)
                || !std::isfinite(item.upstream_frames_per_game_sample)
                || item.upstream_frames_per_game_sample <= 0.0
                || !std::isfinite(item.sample_rate_hz)
                || item.sample_rate_hz <= 0.0
                || !std::isfinite(item.game_pcm_units_per_source_unit)
                || item.game_pcm_units_per_source_unit <= 0.0)
                return false;

            if (item.loop_present) {
                if (item.loop_block_ordinal == snes_studio_source_no_loop
                    || item.loop_block_ordinal >= item.brr_block_count
                    || !std::isfinite(item.upstream_loop_start))
                    return false;
            } else if (item.loop_block_ordinal != snes_studio_source_no_loop
                || item.upstream_loop_start != 0.0) {
                return false;
            }

            const std::uint64_t brr_bytes64 = static_cast<std::uint64_t>(item.brr_block_count)
                * snesapu_brr_bytes_per_block;
            if (brr_bytes64 > spc_ram_size
                || item.brr_offset_bytes != expected_payload
                || brr_bytes64 > size - expected_payload)
                return false;
            const std::size_t brr_bytes = static_cast<std::size_t>(brr_bytes64);
            expected_payload += brr_bytes;

            const std::size_t aligned_pcm = snes_studio_align4(expected_payload);
            if (aligned_pcm < expected_payload || aligned_pcm > size)
                return false;
            for (std::size_t pad = expected_payload; pad < aligned_pcm; ++pad) {
                if (data[pad] != 0u)
                    return false;
            }
            expected_payload = aligned_pcm;

            if (item.pcm_offset_bytes != expected_payload
                || (item.pcm_offset_bytes & 3u) != 0u)
                return false;
            const std::uint64_t pcm_bytes64 = static_cast<std::uint64_t>(item.pcm_frame_count)
                * sizeof(float);
            if (pcm_bytes64 > std::numeric_limits<std::uint32_t>::max()
                || pcm_bytes64 > size - expected_payload)
                return false;
            expected_payload += static_cast<std::size_t>(pcm_bytes64);

            for (std::size_t prior = 0; prior < seen_count; ++prior) {
                if (seen[prior].source_number == item.source_number
                    && seen[prior].first_brr_block_address
                        == item.first_brr_block_address)
                    return false;
            }
            seen[seen_count++] = {
                item.source_number,
                item.first_brr_block_address,
            };
        }

        if (expected_payload != size)
            return false;

        data_ = data;
        size_ = size;
        entry_count_ = entry_count;
        return true;
    }

    [[nodiscard]] bool valid() const noexcept { return data_ != nullptr; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t entry_count() const noexcept { return entry_count_; }

    [[nodiscard]] snes_studio_source_packet_entry_view entry(
        std::size_t index) const noexcept
    {
        if (!valid() || index >= entry_count_)
            return {};
        return entry_unchecked(data_, index);
    }

    [[nodiscard]] const std::uint8_t* brr_bytes(std::size_t index) const noexcept {
        const auto item = entry(index);
        if (item.brr_block_count == 0)
            return nullptr;
        return data_ + item.brr_offset_bytes;
    }

    [[nodiscard]] const std::uint8_t* pcm_bytes(std::size_t index) const noexcept {
        const auto item = entry(index);
        if (item.pcm_frame_count == 0)
            return nullptr;
        return data_ + item.pcm_offset_bytes;
    }

private:
    void clear() noexcept {
        data_ = nullptr;
        size_ = 0;
        entry_count_ = 0;
    }

    static const std::uint8_t* entry_bytes(
        const std::uint8_t* data,
        std::size_t index) noexcept
    {
        return data + snes_studio_source_packet_header_size
            + index * snes_studio_source_packet_entry_size;
    }

    static snes_studio_source_packet_entry_view entry_unchecked(
        const std::uint8_t* data,
        std::size_t index) noexcept
    {
        const std::uint8_t* raw = entry_bytes(data, index);
        snes_studio_source_packet_entry_view item;
        item.source_number = raw[0];
        item.loop_present = (raw[1] & snes_studio_source_flag_loop) != 0u;
        item.first_brr_block_address = snes_studio_read_le16(raw + 2);
        item.brr_block_count = snes_studio_read_le32(raw + 4);
        item.loop_block_ordinal = snes_studio_read_le32(raw + 8);
        item.pcm_frame_count = snes_studio_read_le32(raw + 12);
        item.brr_offset_bytes = snes_studio_read_le32(raw + 16);
        item.pcm_offset_bytes = snes_studio_read_le32(raw + 20);
        item.game_brr_identity.high = snes_studio_read_le64(raw + 24);
        item.game_brr_identity.low = snes_studio_read_le64(raw + 32);
        item.upstream_identity.high = snes_studio_read_le64(raw + 40);
        item.upstream_identity.low = snes_studio_read_le64(raw + 48);
        item.game_origin = snes_studio_read_f64(raw + 56);
        item.upstream_origin = snes_studio_read_f64(raw + 64);
        item.upstream_frames_per_game_sample = snes_studio_read_f64(raw + 72);
        item.upstream_loop_start = snes_studio_read_f64(raw + 80);
        item.sample_rate_hz = snes_studio_read_f64(raw + 88);
        item.game_pcm_units_per_source_unit = snes_studio_read_f64(raw + 96);
        return item;
    }

    const std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t entry_count_ = 0;
};

inline bool snes_studio_brr_headers_match_playback(
    const std::uint8_t* brr,
    std::size_t block_count,
    bool loop_present) noexcept
{
    if (brr == nullptr || block_count == 0)
        return false;
    for (std::size_t block = 0; block < block_count; ++block) {
        const std::uint8_t header = brr[block * snesapu_brr_bytes_per_block];
        const bool end = (header & 0x01u) != 0u;
        const bool loop = (header & 0x02u) != 0u;
        if (block + 1u < block_count) {
            if (end)
                return false;
        } else if (!end || loop != loop_present) {
            return false;
        }
    }
    return true;
}

inline bool snes_studio_brr_matches_spc_snapshot(
    const std::uint8_t* spc_data,
    std::size_t spc_size,
    std::uint16_t first_brr_block_address,
    const std::uint8_t* expected_brr,
    std::size_t expected_brr_size) noexcept
{
    if (!has_spc_signature(spc_data, spc_size)
        || spc_size < spc_min_file_size
        || expected_brr == nullptr
        || expected_brr_size == 0
        || expected_brr_size > spc_ram_size)
        return false;

    for (std::size_t offset = 0; offset < expected_brr_size; ++offset) {
        const std::uint16_t address = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(first_brr_block_address)
            + static_cast<std::uint32_t>(offset));
        if (spc_data[spc_ram_offset + address] != expected_brr[offset])
            return false;
    }
    return true;
}

} // namespace gameaudio::spc
