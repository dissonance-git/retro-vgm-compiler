#pragma once

#include "hes_forensic_capture.h"

#include <cstdint>

// libgme's Classic_Emu.h defines GME_APU_HOOK as a no-op only when callers
// have not supplied one. Force-including this header therefore instruments the
// existing HES write boundary without modifying the pinned third-party source.
#ifndef GME_APU_HOOK
#define GME_APU_HOOK(emu, addr, data) \
    ::vgmtooling::hes::record_apu_write( \
        static_cast<std::int64_t>((emu)->time()), (addr), (data))
#endif
