#include "components/spc/snesapu_prebrr_provider.h"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using namespace gameaudio::spc;

    static_assert((snes_brr_block_bytes * snes_brr_block_inverse_mod_65536)
        % snes_brr_address_modulus == 1u);

    constexpr std::size_t blocks = 4;
    std::array<std::int16_t, blocks * snes_prebrr_samples_per_block> pcm{};
    for (std::size_t block = 0; block < blocks; ++block) {
        for (std::size_t sample = 0; sample < snes_prebrr_samples_per_block; ++sample) {
            pcm[block * snes_prebrr_samples_per_block + sample] =
                static_cast<std::int16_t>(block * 100 + sample);
        }
    }

    snesapu_prebrr_provider<4> provider;
    const snes_prebrr_sample_entry entry{
        7,
        0xfff0,
        static_cast<std::uint32_t>(blocks),
        pcm.data(),
        pcm.size(),
    };
    assert(provider.add(entry));
    assert(provider.count() == 1);
    assert(!provider.add(entry)); // duplicate SRCN is ambiguous

    std::array<std::int16_t, snes_prebrr_samples_per_block> out{};

    // block 0 at FFF0, block 1 at FFF9, block 2 wraps to 0002, block 3 to 000B.
    assert(provider.fill_block(7, 0xfff0, out.data()));
    assert(out[0] == 0 && out[15] == 15);

    assert(provider.fill_block(7, 0xfff9, out.data()));
    assert(out[0] == 100 && out[15] == 115);

    assert(provider.fill_block(7, 0x0002, out.data()));
    assert(out[0] == 200 && out[15] == 215);

    assert(provider.fill_block(7, 0x000b, out.data()));
    assert(out[0] == 300 && out[15] == 315);

    assert(!provider.fill_block(7, 0x0014, out.data())); // fifth block not present
    assert(!provider.fill_block(8, 0xfff0, out.data())); // different source
    assert(!provider.fill_block(7, 0xfff1, out.data())); // not a valid +9 block

    snes_prebrr_sample_entry malformed = entry;
    malformed.prepared_frame_count -= 1;
    snesapu_prebrr_provider<2> invalid;
    assert(!invalid.add(malformed));

    provider.clear();
    assert(provider.count() == 0);
    assert(!provider.fill_block(7, 0xfff0, out.data()));

    return 0;
}
