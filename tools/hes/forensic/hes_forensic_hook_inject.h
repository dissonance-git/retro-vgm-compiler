#pragma once

#include "hes_forensic_capture.h"

#include <cstdint>

// libgme's Classic_Emu.h already owns GME_APU_HOOK at the normal HuC6280 PSG
// boundary. The forensic build force-includes this definition before upstream's
// no-op fallback. A strict pinned-source transform adds the companion ADPCM
// macro at the otherwise unobservable PC Engine CD write boundary.
#ifndef GME_APU_HOOK
#define GME_APU_HOOK(emu, addr, data) \
    ::vgmtooling::hes::record_apu_write( \
        static_cast<std::int64_t>((emu)->time()), (addr), (data))
#endif

#ifndef RETRO_VGM_HES_ADPCM_HOOK
#define RETRO_VGM_HES_ADPCM_HOOK(local_clock, addr, data) \
    ::vgmtooling::hes::record_adpcm_write( \
        static_cast<std::int64_t>(local_clock), (addr), (data))
#endif
