#include "components/spc/snesapu_prebrr_packet.h"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using namespace gameaudio::spc;

    std::array<std::int16_t, 32> a{};
    std::array<std::int16_t, 48> b{};
    for (std::size_t i = 0; i < a.size(); ++i)
        a[i] = static_cast<std::int16_t>(100 + i);
    for (std::size_t i = 0; i < b.size(); ++i)
        b[i] = static_cast<std::int16_t>(-200 + static_cast<int>(i));

    const snes_prebrr_packet_builder::source sources[] = {
        {3, 0x8000, a.data(), a.size()},
        {9, 0xfff9, b.data(), b.size()},
    };

    snes_prebrr_packet_builder builder;
    assert(builder.build(sources, 2));
    assert(!builder.bytes().empty());

    snes_prebrr_packet_view view;
    assert(view.reset(builder.bytes().data(), builder.bytes().size()));
    assert(view.valid());
    assert(view.entry_count() == 2);

    const auto first = view.entry(0);
    assert(first.source_number == 3);
    assert(first.first_brr_block_address == 0x8000);
    assert(first.block_count == 2);
    assert(first.pcm_size_bytes == a.size() * sizeof(std::int16_t));

    const auto second = view.entry(1);
    assert(second.source_number == 9);
    assert(second.first_brr_block_address == 0xfff9);
    assert(second.block_count == 3);

    // The packet is sufficient to construct the realtime provider after one
    // bounded setup-time copy. Playback itself need not parse paths or metadata.
    std::array<std::int16_t, 32> copied_a{};
    const std::uint8_t* raw_a = view.pcm_bytes(0);
    assert(raw_a != nullptr);
    for (std::size_t i = 0; i < copied_a.size(); ++i)
        copied_a[i] = static_cast<std::int16_t>(snes_read_le16(raw_a + i * 2));

    snesapu_prebrr_provider<4> provider;
    assert(provider.add({3, 0x8000, 2, copied_a.data(), copied_a.size()}));
    std::array<std::int16_t, 16> block{};
    assert(provider.fill_block(3, 0x8009, block.data()));
    assert(block[0] == a[16]);
    assert(block[15] == a[31]);

    // Corrupt framing is rejected rather than partially parsed.
    auto corrupt = builder.bytes();
    corrupt[12] ^= 1u; // declared total size
    assert(!view.reset(corrupt.data(), corrupt.size()));

    corrupt = builder.bytes();
    // Duplicate the first SRCN into the second entry.
    corrupt[snes_prebrr_packet_header_size + snes_prebrr_packet_entry_size] = 3;
    assert(!view.reset(corrupt.data(), corrupt.size()));

    snes_prebrr_packet_builder duplicate_builder;
    const snes_prebrr_packet_builder::source duplicate[] = {
        sources[0],
        {3, 0x9000, b.data(), 32},
    };
    assert(!duplicate_builder.build(duplicate, 2));

    const snes_prebrr_packet_builder::source malformed[] = {
        {1, 0x1000, a.data(), 31},
    };
    assert(!duplicate_builder.build(malformed, 1));

    return 0;
}
