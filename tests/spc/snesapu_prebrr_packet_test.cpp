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

    // Child setup owns decoded packet PCM before realtime playback starts.
    snes_prebrr_packet_runtime<4> runtime;
    assert(runtime.load(builder.bytes().data(), builder.bytes().size()));
    assert(runtime.loaded());
    assert(runtime.source_count() == 2);

    std::array<std::int16_t, 16> block{};
    assert(runtime.fill_block(3, 0x8009, block.data()));
    assert(block[0] == a[16]);
    assert(block[15] == a[31]);

    // The same method has the exact C-callback shape the spcplayer wrapper calls.
    block.fill(0);
    assert(snes_prebrr_packet_runtime<4>::callback(
        &runtime, 9, 0x0002, block.data()) == 1); // FFF9 + 9 wraps to 0002
    assert(block[0] == b[16]);
    assert(block[15] == b[31]);
    assert(snes_prebrr_packet_runtime<4>::callback(
        &runtime, 1, 0x0002, block.data()) == 0);
    assert(snes_prebrr_packet_runtime<4>::callback(
        nullptr, 9, 0x0002, block.data()) == 0);

    // Corrupt framing is rejected rather than partially parsed.
    auto corrupt = builder.bytes();
    corrupt[12] ^= 1u; // declared total size
    assert(!view.reset(corrupt.data(), corrupt.size()));
    assert(!runtime.load(corrupt.data(), corrupt.size()));
    assert(!runtime.loaded());

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
