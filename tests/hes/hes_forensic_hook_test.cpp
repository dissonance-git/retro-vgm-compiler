#include "tools/hes/forensic/hes_forensic_capture.h"

#include <gme/gme.h>

#include <cassert>
#include <cstdint>
#include <vector>

using vgmtooling::hes::apu_write_observation;
using vgmtooling::hes::record_apu_write;
using vgmtooling::hes::scoped_apu_write_capture;

int main() {
    // Referencing the real HES type pulls the pinned Hes_Emu object through the
    // static linker. That object is compiled with the force-injected
    // GME_APU_HOOK, so this test also verifies the upstream hook contract links
    // against the first-party recorder without modifying libgme source.
    assert(gme_hes_type != nullptr);

    std::vector<apu_write_observation> outer;
    std::vector<apu_write_observation> nested;

    {
        scoped_apu_write_capture capture(outer);
        record_apu_write(100, 0x00, 0x03); // channel latch
        record_apu_write(101, 0x02, 0x34); // frequency low

        {
            scoped_apu_write_capture nested_capture(nested);
            record_apu_write(102, 0x03, 0x02); // frequency high
            record_apu_write(103, 0x04, 0x9F); // control
        }

        record_apu_write(104, 0x05, 0xFF); // balance
        record_apu_write(105, 0x0A, 0x7F); // outside HES PSG register range
    }

    // No active capture after scope exit.
    record_apu_write(106, 0x06, 0x1F);

    assert(outer.size() == 3);
    assert(outer[0].clock == 100);
    assert(outer[0].register_offset == 0x00);
    assert(outer[0].data == 0x03);
    assert(outer[1].clock == 101);
    assert(outer[1].register_offset == 0x02);
    assert(outer[1].data == 0x34);
    assert(outer[2].clock == 104);
    assert(outer[2].register_offset == 0x05);
    assert(outer[2].data == 0xFF);

    assert(nested.size() == 2);
    assert(nested[0].clock == 102);
    assert(nested[0].register_offset == 0x03);
    assert(nested[0].data == 0x02);
    assert(nested[1].clock == 103);
    assert(nested[1].register_offset == 0x04);
    assert(nested[1].data == 0x9F);

    return 0;
}
