import unittest
from pathlib import Path


class StudioSourceTransportPatchTest(unittest.TestCase):
    def test_spcp_v3_transport_contract_is_pinned(self):
        repo = Path(__file__).resolve().parents[2]
        script = repo / "patches" / "snesapu" / "apply_studio_source_transport.py"
        text = script.read_text(encoding="utf-8")

        # Keep the patch itself syntactically executable while also pinning the
        # exact v2 -> v3 wire-format transition. These strings are deliberately
        # the guarded predecessor/successor anchors used by replace_once().
        compile(text, str(script), "exec")
        self.assertIn('#define SPCP_HEADER_VERSION 2\\n', text)
        self.assertIn('#define SPCP_HEADER_VERSION 3\\n', text)
        self.assertIn("SPCP_HEADER_PREBRR_SIZE_OFFSET 20", text)
        self.assertIn("SPCP_HEADER_STUDIO_SIZE_OFFSET 24", text)
        self.assertIn("28 remains reserved and must be zero", text)
        self.assertIn("m_prebrr_size + m_studio_source_size", text)
        self.assertIn("+ static_cast<uint64_t>(studio_source_size)", text)
        self.assertIn("const uint8_t* studio_source_data = prebrr_data + prebrr_size", text)

        # The child must validate the studio packet against the actual SPC RAM
        # image before InitAPU and prepare the expensive FIR table before audio.
        self.assertIn(
            "studio_runtime.load(studio_source_data, studio_source_size, spc_data, spc_size)",
            text,
        )
        self.assertIn("Invalid verified studio-source packet or BRR witness mismatch", text)
        self.assertLess(text.index("studio_runtime.load("), text.index("InitAPU();"))
        self.assertIn("prepare_spc_studio_sample_reconstruction();", text)
        self.assertIn('GetProcAddress(module, "SetDSPStudioSourceProvider")', text)
        self.assertIn("&retro_studio_begin, &retro_studio_sample, &studio_runtime", text)

        # The hot callback ABI must preserve the live source-topology facts
        # computed by the SNESAPU assembly seam. DIR alone is insufficient: the
        # pinned END+LOOP path may refresh DSP SRCN, apply Script700 NoteChange,
        # and select a different directory loop word while the page is unchanged.
        self.assertIn("using RetroStudioSampleCallback = u32 (__stdcall *)(", text)
        self.assertIn("u32 effective_srcn", text)
        self.assertIn("u32 live_loop_brr", text)
        callback = text.index("static u32 __stdcall retro_studio_sample(")
        forwarding = text.index("runtime->provider().render_voice(", callback)
        self.assertLess(callback, forwarding)
        for argument in (
            "m_rate_q16_16",
            "effective_srcn",
            "live_loop_brr",
            "directory_page",
            "interpolation",
            "out_sample",
        ):
            self.assertIn(argument, text[forwarding : forwarding + 400])

        # Sidecar discovery stays setup-only, local-file-only and bounded. A
        # missing top-rung source is normal and leaves the lower ladder alive.
        self.assertIn('sidecar += ".studiosrc"', text)
        self.assertIn("cfg_enhanced_enabled", text)
        self.assertIn("!filesystem::g_is_remote_or_unrecognized", text)
        self.assertIn("size64 <= 256u * 1024u * 1024u", text)
        self.assertIn("Missing top-rung source is normal", text)

        # Every child-local dependency named by the patch must still exist in
        # the repository. This catches transport drift when source headers move.
        overlay_names = (
            "spc_sample_restoration.h",
            "spc_snesapu_source_trajectory.h",
            "spc_studio_sample_reconstruction.h",
            "spc_upstream_playback_reconstruction.h",
            "snesapu_studio_source_provider.h",
            "spc_snapshot.h",
            "snesapu_studio_source_packet.h",
            "snesapu_studio_source_packet_runtime.h",
        )
        for name in overlay_names:
            self.assertTrue((repo / "components" / "spc" / name).is_file(), name)
            self.assertIn(f'"{name}"', text)


if __name__ == "__main__":
    unittest.main()
