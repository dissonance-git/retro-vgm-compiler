#pragma once

#include "snesapu_prebrr_provider.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace gameaudio::spc {

constexpr std::uint32_t snes_prebrr_packet_magic = 0x52425250u; // "PRBR" LE
constexpr std::uint16_t snes_prebrr_packet_version = 1u;
constexpr std::size_t snes_prebrr_packet_header_size = 16u;
constexpr std::size_t snes_prebrr_packet_entry_size = 16u;

struct snes_prebrr_packet_entry_view {
    std::uint8_t source_number = 0;
    std::uint16_t first_brr_block_address = 0;
    std::uint32_t block_count = 0;
    std::uint32_t pcm_offset_bytes = 0;
    std::uint32_t pcm_size_bytes = 0;
};

inline std::uint16_t snes_read_le16(const std::uint8_t* data) noexcept {
    return static_cast<std::uint16_t>(data[0])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) << 8u);
}

inline std::uint32_t snes_read_le32(const std::uint8_t* data) noexcept {
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8u)
        | (static_cast<std::uint32_t>(data[2]) << 16u)
        | (static_cast<std::uint32_t>(data[3]) << 24u);
}

inline void snes_write_le16(std::uint8_t* data, std::uint16_t value) noexcept {
    data[0] = static_cast<std::uint8_t>(value);
    data[1] = static_cast<std::uint8_t>(value >> 8u);
}

inline void snes_write_le32(std::uint8_t* data, std::uint32_t value) noexcept {
    data[0] = static_cast<std::uint8_t>(value);
    data[1] = static_cast<std::uint8_t>(value >> 8u);
    data[2] = static_cast<std::uint8_t>(value >> 16u);
    data[3] = static_cast<std::uint8_t>(value >> 24u);
}

class snes_prebrr_packet_view {
public:
    bool reset(const std::uint8_t* data, std::size_t size) noexcept {
        data_ = nullptr;
        size_ = 0;
        entry_count_ = 0;
        if (data == nullptr || size < snes_prebrr_packet_header_size)
            return false;
        if (snes_read_le32(data) != snes_prebrr_packet_magic
            || snes_read_le16(data + 4) != snes_prebrr_packet_version
            || snes_read_le16(data + 6) != snes_prebrr_packet_header_size)
            return false;

        const std::uint16_t entry_count = snes_read_le16(data + 8);
        const std::uint16_t reserved = snes_read_le16(data + 10);
        const std::uint32_t declared_size = snes_read_le32(data + 12);
        if (reserved != 0 || declared_size != size)
            return false;

        const std::size_t table_bytes = static_cast<std::size_t>(entry_count)
            * snes_prebrr_packet_entry_size;
        if (table_bytes > size - snes_prebrr_packet_header_size)
            return false;
        const std::size_t payload_floor = snes_prebrr_packet_header_size + table_bytes;

        bool source_seen[256]{};
        for (std::size_t index = 0; index < entry_count; ++index) {
            const auto entry = entry_unchecked(data, index);
            if (source_seen[entry.source_number])
                return false;
            source_seen[entry.source_number] = true;
            if (entry.block_count == 0
                || entry.pcm_size_bytes
                    != entry.block_count * snes_prebrr_samples_per_block * sizeof(std::int16_t)
                || entry.pcm_offset_bytes < payload_floor
                || (entry.pcm_offset_bytes & 1u) != 0u
                || entry.pcm_offset_bytes > size
                || entry.pcm_size_bytes > size - entry.pcm_offset_bytes)
                return false;
        }

        data_ = data;
        size_ = size;
        entry_count_ = entry_count;
        return true;
    }

    [[nodiscard]] bool valid() const noexcept { return data_ != nullptr; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t entry_count() const noexcept { return entry_count_; }

    [[nodiscard]] snes_prebrr_packet_entry_view entry(std::size_t index) const noexcept {
        if (!valid() || index >= entry_count_)
            return {};
        return entry_unchecked(data_, index);
    }

    [[nodiscard]] const std::uint8_t* pcm_bytes(std::size_t index) const noexcept {
        const auto item = entry(index);
        if (item.block_count == 0)
            return nullptr;
        return data_ + item.pcm_offset_bytes;
    }

private:
    static snes_prebrr_packet_entry_view entry_unchecked(
        const std::uint8_t* data,
        std::size_t index) noexcept
    {
        const std::uint8_t* raw = data + snes_prebrr_packet_header_size
            + index * snes_prebrr_packet_entry_size;
        snes_prebrr_packet_entry_view item;
        item.source_number = raw[0];
        item.first_brr_block_address = snes_read_le16(raw + 2);
        item.block_count = snes_read_le32(raw + 4);
        item.pcm_offset_bytes = snes_read_le32(raw + 8);
        item.pcm_size_bytes = snes_read_le32(raw + 12);
        return item;
    }

    const std::uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t entry_count_ = 0;
};

// Small builder used by the foobar parent after the offline lineage system has
// already approved one or more samples. It serializes only prepared game-grid
// PCM, never corpus/library paths. Thus the spcplayer child remains independent
// of the user's archival layout and receives exactly the data needed for one
// playback process.
class snes_prebrr_packet_builder {
public:
    struct source {
        std::uint8_t source_number = 0;
        std::uint16_t first_brr_block_address = 0;
        const std::int16_t* prepared_pcm = nullptr;
        std::size_t prepared_frame_count = 0;
    };

    bool build(const source* sources, std::size_t count) {
        bytes_.clear();
        if ((count != 0 && sources == nullptr)
            || count > 256
            || count > std::numeric_limits<std::uint16_t>::max())
            return false;

        bool seen[256]{};
        std::size_t total = snes_prebrr_packet_header_size
            + count * snes_prebrr_packet_entry_size;
        for (std::size_t index = 0; index < count; ++index) {
            const source& item = sources[index];
            if (seen[item.source_number]
                || item.prepared_pcm == nullptr
                || item.prepared_frame_count == 0
                || item.prepared_frame_count % snes_prebrr_samples_per_block != 0)
                return false;
            seen[item.source_number] = true;
            const std::size_t bytes = item.prepared_frame_count * sizeof(std::int16_t);
            if (bytes > std::numeric_limits<std::uint32_t>::max()
                || total > std::numeric_limits<std::uint32_t>::max() - bytes)
                return false;
            total += bytes;
        }
        if (total > std::numeric_limits<std::uint32_t>::max())
            return false;

        bytes_.assign(total, 0);
        snes_write_le32(bytes_.data(), snes_prebrr_packet_magic);
        snes_write_le16(bytes_.data() + 4, snes_prebrr_packet_version);
        snes_write_le16(bytes_.data() + 6, snes_prebrr_packet_header_size);
        snes_write_le16(bytes_.data() + 8, static_cast<std::uint16_t>(count));
        snes_write_le16(bytes_.data() + 10, 0);
        snes_write_le32(bytes_.data() + 12, static_cast<std::uint32_t>(total));

        std::size_t payload = snes_prebrr_packet_header_size
            + count * snes_prebrr_packet_entry_size;
        for (std::size_t index = 0; index < count; ++index) {
            const source& item = sources[index];
            std::uint8_t* raw = bytes_.data() + snes_prebrr_packet_header_size
                + index * snes_prebrr_packet_entry_size;
            raw[0] = item.source_number;
            raw[1] = 0;
            snes_write_le16(raw + 2, item.first_brr_block_address);
            const std::size_t block_count = item.prepared_frame_count
                / snes_prebrr_samples_per_block;
            snes_write_le32(raw + 4, static_cast<std::uint32_t>(block_count));
            snes_write_le32(raw + 8, static_cast<std::uint32_t>(payload));
            const std::size_t pcm_size = item.prepared_frame_count * sizeof(std::int16_t);
            snes_write_le32(raw + 12, static_cast<std::uint32_t>(pcm_size));
            std::memcpy(bytes_.data() + payload, item.prepared_pcm, pcm_size);
            payload += pcm_size;
        }
        return true;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept {
        return bytes_;
    }

private:
    std::vector<std::uint8_t> bytes_;
};

} // namespace gameaudio::spc
