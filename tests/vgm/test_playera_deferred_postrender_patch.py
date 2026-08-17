import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


HEADER_FIXTURE = '''#define LIBVGM_GAMEAUDIO_POSTRENDER_ABI 1
#ifndef __PLAYERA_HPP__
#define __PLAYERA_HPP__
#include <vector>
class PlayerA
{
public:
\ttypedef void (*PLR_SMPL_PACK)(void* buffer, INT32 value);
\ttypedef void (*PLR_POST_RENDER_PROCESSOR)(void* user, WAVE_32BS* samples, UINT32 sampleCount, UINT32 basePlaybackSample);
\tvoid SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam);
private:
\tPLR_POST_RENDER_PROCESSOR _postRenderFunc;
\tvoid* _postRenderParam;
\tPLR_SMPL_PACK _outSmplPack;
\tstd::vector<WAVE_32BS> _smplBuf;
};
#endif
'''


CPP_FIXTURE = '''PlayerA::PlayerA()
{
\t_postRenderFunc = NULL;
\t_postRenderParam = NULL;
\t_myPlayState = 0x00;
}

UINT8 PlayerA::SetOutputSettings(UINT32 smplRate, UINT8 channels, UINT8 smplBits, UINT32 smplBufferLen)
{
\t_smplBuf.resize(smplBufferLen);
\treturn 0x00;
}

void PlayerA::SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam)
{
\t_postRenderFunc = cbFunc;
\t_postRenderParam = cbParam;
\treturn;
}

UINT8 PlayerA::GetState(void) const
{
\treturn _myPlayState;
}

UINT8 PlayerA::Start(void)
{
\tUINT8 retVal = _player->Start();
\t_myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);
\treturn retVal;
}

UINT8 PlayerA::Stop(void)
{
\treturn _player->Stop();
}

UINT8 PlayerA::Reset(void)
{
\tUINT8 retVal = _player->Reset();
\t_myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);
\treturn retVal;
}

UINT8 PlayerA::FadeOut(void)
{
\treturn 0;
}

UINT8 PlayerA::Seek(UINT8 unit, UINT32 pos)
{
\tUINT8 retVal = _player->Seek(unit, pos);
\t_myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);
\tUINT32 pbSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);
\tif (pbSmpl < _fadeSmplStart)
\t\t_fadeSmplStart = (UINT32)-1;
\tif (pbSmpl < _endSilenceStart)
\t\t_endSilenceStart = (UINT32)-1;
\treturn retVal;
}

#if 1
#define VOLCALC64

UINT32 PlayerA::Render(UINT32 bufSize, void* data)
{
\treturn 0;
}

/*static*/ UINT8 PlayerA::PlayCallbackS(PlayerBase* player, void* userParam, UINT8 evtType, void* evtParam)
{
\treturn 0;
}
'''


class PlayerADeferredPostRenderPatchTest(unittest.TestCase):
    def test_deferred_boundary_remains_inside_playera_output_pipeline(self) -> None:
        root = Path(__file__).resolve().parents[2]
        patch = root / "patches/libvgm/apply_playera_deferred_postrender.py"

        with tempfile.TemporaryDirectory() as tmp:
            libvgm = Path(tmp)
            player = libvgm / "player"
            player.mkdir()
            hpp = player / "playera.hpp"
            cpp = player / "playera.cpp"
            hpp.write_text(HEADER_FIXTURE, encoding="utf-8")
            cpp.write_text(CPP_FIXTURE, encoding="utf-8")
            subprocess.run(
                [sys.executable, "-B", str(patch), str(libvgm)],
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            header = hpp.read_text(encoding="utf-8")
            source = cpp.read_text(encoding="utf-8")

        self.assertIn("LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI", header)
        self.assertIn("PLR_DEFERRED_POST_RENDER_PROCESSOR", header)
        self.assertIn("SetDeferredPostRenderProcessor", header)
        self.assertIn("_deferredRenderBuf", header)
        self.assertIn("_deferredRenderBuf.resize(smplBufferLen);", source)
        self.assertIn(
            "PLAYSTATE_END by\n\t// itself is not EOF here because chip release tails remain renderable.",
            header,
        )

        render_start = source.index("UINT32 PlayerA::Render")
        render_end = source.index("/*static*/ UINT8 PlayerA::PlayCallbackS", render_start)
        render = source[render_start:render_end]
        engine = render.index("smplRendered = _player->Render(renderRequest")
        post = render.index("_postRenderFunc(", engine)
        deferred = render.index("cbResult = _deferredPostRenderFunc(", post)
        volume = render.index("curVolume = CalcCurrentVolume(basePbSmpl)", deferred)
        packing = render.index("_outSmplPack(", volume)
        self.assertLess(engine, post)
        self.assertLess(post, deferred)
        self.assertLess(deferred, volume)
        self.assertLess(volume, packing)

        self.assertIn("basePbSmpl = _deferredEmitSmpl;", render)
        self.assertIn("_deferredEmitSmpl + finalized", render)
        self.assertIn("_deferredEmitSmpl += curSmpl;", render)
        self.assertIn("passes < 16", render)

        # VGMPlayer keeps PLAYSTATE_PLAY set after PLAYSTATE_END so chip release
        # tails remain renderable. The FIR must not flush its post-roll merely
        # because the command stream reached end-of-data.
        self.assertEqual(
            render.count("sourceEnded = (state & PLAYSTATE_PLAY) ? 0 : 1;"),
            2,
        )
        source_ended_lines = [
            line for line in render.splitlines() if "sourceEnded =" in line
        ]
        self.assertTrue(source_ended_lines)
        self.assertTrue(all("PLAYSTATE_END" not in line for line in source_ended_lines))

        start = source[source.index("UINT8 PlayerA::Start"):source.index("UINT8 PlayerA::Stop")]
        reset = source[source.index("UINT8 PlayerA::Reset"):source.index("UINT8 PlayerA::FadeOut")]
        seek = source[source.index("UINT8 PlayerA::Seek"):source.index("#if 1")]
        self.assertIn("_deferredEmitSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);", start)
        self.assertIn("_deferredEmitSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);", reset)
        self.assertIn("_deferredEmitSmpl = pbSmpl;", seek)

    def test_source_capture_patch_order(self) -> None:
        root = Path(__file__).resolve().parents[2]
        chain = (root / "patches/libvgm/apply_source_capture.py").read_text(
            encoding="utf-8"
        )
        ordinary = chain.index('run(here / "apply_playera_postrender_hook.py", root)')
        deferred = chain.index('run(here / "apply_playera_deferred_postrender.py", root)')
        self.assertLess(ordinary, deferred)


if __name__ == "__main__":
    unittest.main()
