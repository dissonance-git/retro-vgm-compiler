from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "components" / "vgm" / "foo_input_vgm" / "src" / "input_vgm.h"
SHADOW = ROOT / "components" / "vgm" / "foo_input_vgm" / "src" / "input_vgm_shadow.cpp"


class QSoundLiveMixWiringTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = HEADER.read_text(encoding="utf-8")
        cls.shadow = SHADOW.read_text(encoding="utf-8")

    def test_input_owns_bounded_native_mix_sidecar(self):
        self.assertIn("qsound_native_mix_capture.h", self.header)
        self.assertIn("m_qsound_mix_capture", self.header)
        self.assertIn("m_qsound_mix_shadow_valid", self.header)
        self.assertIn("m_qsound_mix_capture_active", self.header)

    def test_callback_preserves_freshness_and_exact_native_terms(self):
        self.assertIn("frame.native_sample = event->nativeSample;", self.shadow)
        self.assertIn("frame.accounting_valid = event->accountingValid != 0;", self.shadow)
        self.assertIn("frame.echo_input = event->echoInput;", self.shadow)
        self.assertIn("frame.echo_output = event->echoOutput;", self.shadow)
        self.assertIn("frame.wet_post_delay[ch] = event->wetPostDelay[ch];", self.shadow)
        self.assertIn("frame.dry_post_delay[ch] = event->dryPostDelay[ch];", self.shadow)
        self.assertIn("frame.reference_output[ch] = event->output[ch];", self.shadow)

    def test_mix_observer_wraps_reference_decode_only(self):
        attach = self.shadow.find(
            "SetQSoundMixObserver(&input_vgm::qsound_mix_callback, this)"
        )
        reference_decode = self.shadow.find("input_base::decode_run(p_chunk, p_abort)")
        self.assertGreaterEqual(attach, 0)
        self.assertGreater(reference_decode, attach)
        self.assertGreaterEqual(
            self.shadow.count("SetQSoundMixObserver(nullptr, nullptr)"), 3
        )

    def test_seek_never_attaches_discarded_mix_capture(self):
        seek = self.shadow.find("void input_vgm::decode_seek")
        self.assertGreaterEqual(seek, 0)
        seek_text = self.shadow[seek:]
        self.assertNotIn(
            "SetQSoundMixObserver(&input_vgm::qsound_mix_callback, this)",
            seek_text,
        )
        self.assertIn("m_qsound_mix_capture.begin_block();", seek_text)

    def test_source_and_mix_sidecars_fail_closed_on_timeline_disagreement(self):
        self.assertIn(
            "m_qsound_audio_capture.native_sample_rate() == m_qsound_mix_capture.native_sample_rate()",
            self.shadow,
        )
        self.assertIn(
            "m_qsound_audio_capture.count() == m_qsound_mix_capture.count()",
            self.shadow,
        )
        self.assertIn(
            "m_qsound_audio_capture.first_native_sample() == m_qsound_mix_capture.first_native_sample()",
            self.shadow,
        )
        self.assertIn("m_qsound_audio_shadow_valid = false;", self.shadow)
        self.assertIn("m_qsound_mix_shadow_valid = false;", self.shadow)

    def test_sidecars_do_not_rewrite_audible_chunk(self):
        self.assertNotIn("m_qsound_mix_capture.frames()[", self.shadow)
        self.assertNotIn("p_chunk.set_data", self.shadow)
        self.assertIn(
            "p_chunk is still produced solely by the historical reference path",
            self.shadow,
        )


if __name__ == "__main__":
    unittest.main()
