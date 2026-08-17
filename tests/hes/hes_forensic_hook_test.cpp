#include "tools/hes/forensic/hes_forensic_capture.h"

#include <gme/gme.h>

#include <cassert>
#include <cstdint>
#include <vector>

using vgmtooling::hes::adpcm_write_observation;
using vgmtooling::hes::apu_write_observation;
using vgmtooling::hes::record_adpcm_write;
using vgmtooling::hes::record_apu_write;
using vgmtooling::hes::scoped_adpcm_write_capture;
using vgmtooling::hes::scoped_apu_write_capture;
using vgmtooling::hes::set_adpcm_write_epoch;
using vgmtooling::hes::set_apu_write_epoch;

int main() {
    // Referencing the real HES type pulls the pinned Hes_Emu object through the
    // static linker. That object is compiled with the force-injected PSG hook
    // and strict ADPCM hook patch, so this also verifies both upstream-facing
    // observation contracts resolve against the first-party recorder.
    assert(gme_hes_type != nullptr);

    std::vector<apu_write_observation> outer;
    std::vector<apu_write_observation> nested;
    std::vector<adpcm_write_observation> adpcm;

    {
        scoped_apu_write_capture capture(outer);
        scoped_adpcm_write_capture adpcm_capture(adpcm);
        set_apu_write_epoch(1000);
        set_adpcm_write_epoch(1000);
        record_apu_write(100, 0x00, 0x03); // channel latch
        record_apu_write(101, 0x02, 0x34); // frequency low
        record_adpcm_write(101, 0x20, 0x55);

        {
            scoped_apu_write_capture nested_capture(nested);
            set_apu_write_epoch(4000);
            record_apu_write(102, 0x03, 0x02); // frequency high
            record_apu_write(103, 0x04, 0x9F); // control
        }

        // Nested PSG scope restores the outer PSG epoch. ADPCM remains on the
        // independently active outer lane.
        record_apu_write(104, 0x05, 0xFF); // balance
        record_adpcm_write(104, 0x3FF, 0xAA);
        record_adpcm_write(105, 0x400, 0xBB); // outside ADPCM register range
        record_apu_write(105, 0x0A, 0x7F); // outside HES PSG register range
    }

    // No active capture after scope exit.
    record_apu_write(106, 0x06, 0x1F);
    record_adpcm_write(106, 0x01, 0x12);

    assert(outer.size() == 3);
    assert(outer[0].clock == 1100);
    assert(outer[0].register_offset == 0x00);
    assert(outer[0].data == 0x03);
    assert(outer[1].clock == 1101);
    assert(outer[1].register_offset == 0x02);
    assert(outer[1].data == 0x34);
    assert(outer[2].clock == 1104);
    assert(outer[2].register_offset == 0x05);
    assert(outer[2].data == 0xFF);

    assert(nested.size() == 2);
    assert(nested[0].clock == 4102);
    assert(nested[0].register_offset == 0x03);
    assert(nested[0].data == 0x02);
    assert(nested[1].clock == 4103);
    assert(nested[1].register_offset == 0x04);
    assert(nested[1].data == 0x9F);

    assert(adpcm.size() == 2);
    assert(adpcm[0].clock == 1101);
    assert(adpcm[0].register_offset == 0x20);
    assert(adpcm[0].data == 0x55);
    assert(adpcm[1].clock == 1104);
    assert(adpcm[1].register_offset == 0x3FF);
    assert(adpcm[1].data == 0xAA);

    return 0;
}
