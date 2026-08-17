import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


HEADER_FIXTURE = r'''#define LIBVGM_GAMEAUDIO_POSTRENDER_ABI 1
#ifndef __PLAYERA_HPP__
#define __PLAYERA_HPP__
#include <vector>
class PlayerA
{
public:
    typedef void (*PLR_SMPL_PACK)(void* buffer, INT32 value);
    typedef void (*PLR_POST_RENDER_PROCESSOR)(void* user, WAVE_32BS* samples, UINT32 sampleCount, UINT32 basePlaybackSample);
    void SetLogCallback(PLAYER_LOG_CB cbFunc, void* cbParam);
    void SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam);
    UINT32 Render(UINT32 bufSize, void* data);
private:
    PLAYER_EVENT_CB _plrCbFunc;
    void* _plrCbParam;
    PLR_POST_RENDER_PROCESSOR _postRenderFunc;
    void* _postRenderParam;
    PLR_SMPL_PACK _outSmplPack;
    std::vector<WAVE_32BS> _smplBuf;
};
#endif
'''


CPP_FIXTURE = r'''PlayerA::PlayerA()
{
    _postRenderFunc = NULL;
    _postRenderParam = NULL;
    _myPlayState = 0x00;
}

UINT8 PlayerA::SetOutputSettings(UINT32 smplRate, UINT8 channels, UINT8 smplBits, UINT32 smplBufferLen)
{
    _smplBuf.resize(smplBufferLen);
    return 0x00;
}

void PlayerA::SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam)
{
    _postRenderFunc = cbFunc;
    _postRenderParam = cbParam;
    return;
}

UINT8 PlayerA::GetState(void) const
{
    return _myPlayState;
}

UINT8 PlayerA::Start(void)
{
    UINT8 retVal = _player->Start();
    _myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);
    return retVal;
}

UINT8 PlayerA::Stop(void)
{
    return _player->Stop();
}

UINT8 PlayerA::Reset(void)
{
    UINT8 retVal = _player->Reset();
    _myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);
    return retVal;
}

UINT8 PlayerA::FadeOut(void)
{
    return 0;
}

UINT8 PlayerA::Seek(UINT8 unit, UINT32 pos)
{
    UINT8 retVal = _player->Seek(unit, pos);
    _myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);
    UINT32 pbSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);
    if (pbSmpl < _fadeSmplStart)
        _fadeSmplStart = (UINT32)-1;
    if (pbSmpl < _endSilenceStart)
        _endSilenceStart = (UINT32)-1;
    return retVal;
}

#if 1
#define VOLCALC64
#define VOL_BITS 16
#define VOL_SHIFT (16 - VOL_BITS)

UINT32 PlayerA::Render(UINT32 bufSize, void* data)
{
    UINT8* bData = (UINT8*)data;
    UINT32 basePbSmpl;
    UINT32 smplCount;
    UINT32 smplRendered;
    UINT32 curSmpl;
    WAVE_32BS fnlSmpl;
    INT32 curVolume;
    smplCount = bufSize / _outSmplSizeA;
    if (_player == NULL) return 0;
    if (! (_player->GetState() & PLAYSTATE_PLAY)) return 0;
    if (! smplCount) return 0;
    if (smplCount > (UINT32)_smplBuf.size()) smplCount = (UINT32)_smplBuf.size();
    memset(&_smplBuf[0], 0, smplCount * sizeof(WAVE_32BS));
    basePbSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);
    smplRendered = _player->Render(smplCount, &_smplBuf[0]);
    smplCount = smplRendered;
    if (_postRenderFunc != NULL && smplCount != 0)
        _postRenderFunc(_postRenderParam, &_smplBuf[0], smplCount, basePbSmpl);
    curVolume = CalcCurrentVolume(basePbSmpl) >> VOL_SHIFT;
    for (curSmpl = 0; curSmpl < smplCount; curSmpl ++, basePbSmpl ++)
    {
        fnlSmpl = _smplBuf[curSmpl];
        _outSmplPack(&bData[(curSmpl * 2 + 0) * _outSmplSize1], fnlSmpl.L);
        _outSmplPack(&bData[(curSmpl * 2 + 1) * _outSmplSize1], fnlSmpl.R);
    }
    return curSmpl * _outSmplSizeA;
}

/*static*/ UINT8 PlayerA::PlayCallbackS(PlayerBase* player, void* userParam, UINT8 evtType, void* evtParam)
{
    return 0;
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
        self.assertIn("return 0;", render)

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
