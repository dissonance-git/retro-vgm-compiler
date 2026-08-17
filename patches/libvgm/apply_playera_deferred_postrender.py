#!/usr/bin/env python3
"""Add a bounded render-ahead seam between PlayerBase and PlayerA output DSP.

The existing post-render hook exposes PlayerBase's WAVE_32BS block before
PlayerA song gain, fade, inversion, clipping, and packing. Studio FM needs a
small amount of future source support, so a block-local callback is not enough.

This guarded patch adds a second, optional processor. PlayerA may render future
engine blocks through the ordinary post-render hook, hand them to the deferred
processor, and wait until that processor returns the oldest finalized source
ordinals. PlayerA then applies its unchanged downstream output pipeline using
those finalized ordinals. No output-format or fade logic is duplicated outside
PlayerA.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


def replace_between_once(text: str, start: str, end: str, body: str, label: str) -> str:
    if text.count(start) != 1 or text.count(end) != 1:
        raise RuntimeError(
            f"{label}: expected singular boundary markers, found "
            f"start={text.count(start)} end={text.count(end)}"
        )
    begin = text.index(start)
    finish = text.index(end, begin)
    return text[:begin] + body + text[finish:]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("libvgm_root", type=Path)
    args = parser.parse_args()
    root = args.libvgm_root.resolve()
    hpp = root / "player" / "playera.hpp"
    cpp = root / "player" / "playera.cpp"

    h = hpp.read_text(encoding="utf-8")
    if "LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI" not in h:
        h = "#define LIBVGM_GAMEAUDIO_DEFERRED_POSTRENDER_ABI 1\n" + h

    h = replace_once(
        h,
        "\ttypedef void (*PLR_POST_RENDER_PROCESSOR)(void* user, WAVE_32BS* samples, UINT32 sampleCount, UINT32 basePlaybackSample);\n",
        "\ttypedef void (*PLR_POST_RENDER_PROCESSOR)(void* user, WAVE_32BS* samples, UINT32 sampleCount, UINT32 basePlaybackSample);\n"
        "\t// Optional non-causal processor. Newly rendered engine samples are in\n"
        "\t// pre-volume WAVE_32BS space. The callback may retain them and return\n"
        "\t// only a finalized prefix beginning at expectedFinalizedBase. A quality\n"
        "\t// failure must be resolved inside the callback as protected-reference\n"
        "\t// output; nonzero return values are reserved for contract corruption.\n"
        "\ttypedef UINT8 (*PLR_DEFERRED_POST_RENDER_PROCESSOR)(\n"
        "\t\tvoid* user,\n"
        "\t\tconst WAVE_32BS* renderedSamples,\n"
        "\t\tUINT32 renderedCount,\n"
        "\t\tUINT32 renderedBasePlaybackSample,\n"
        "\t\tWAVE_32BS* finalizedSamples,\n"
        "\t\tUINT32 finalizedCapacity,\n"
        "\t\tUINT32 expectedFinalizedBase,\n"
        "\t\tUINT8 sourceEnded,\n"
        "\t\tUINT32* finalizedCount);\n",
        "PlayerA deferred callback typedef",
    )

    h = replace_once(
        h,
        "\tvoid SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam);\n",
        "\tvoid SetPostRenderProcessor(PLR_POST_RENDER_PROCESSOR cbFunc, void* cbParam);\n"
        "\tvoid SetDeferredPostRenderProcessor(PLR_DEFERRED_POST_RENDER_PROCESSOR cbFunc, void* cbParam);\n",
        "PlayerA deferred setter declaration",
    )

    h = replace_once(
        h,
        "\tPLR_POST_RENDER_PROCESSOR _postRenderFunc;\n\tvoid* _postRenderParam;\n",
        "\tPLR_POST_RENDER_PROCESSOR _postRenderFunc;\n\tvoid* _postRenderParam;\n"
        "\tPLR_DEFERRED_POST_RENDER_PROCESSOR _deferredPostRenderFunc;\n"
        "\tvoid* _deferredPostRenderParam;\n"
        "\tUINT32 _deferredEmitSmpl;\n",
        "PlayerA deferred callback state",
    )

    h = replace_once(
        h,
        "\tstd::vector<WAVE_32BS> _smplBuf;\n",
        "\tstd::vector<WAVE_32BS> _smplBuf;\n"
        "\tstd::vector<WAVE_32BS> _deferredRenderBuf;\n",
        "PlayerA deferred engine buffer",
    )

    c = cpp.read_text(encoding="utf-8")
    c = replace_once(
        c,
        "\t_postRenderFunc = NULL;\n\t_postRenderParam = NULL;\n",
        "\t_postRenderFunc = NULL;\n\t_postRenderParam = NULL;\n"
        "\t_deferredPostRenderFunc = NULL;\n"
        "\t_deferredPostRenderParam = NULL;\n"
        "\t_deferredEmitSmpl = 0;\n",
        "PlayerA deferred initialization",
    )

    c = replace_once(
        c,
        "\t_smplBuf.resize(smplBufferLen);\n",
        "\t_smplBuf.resize(smplBufferLen);\n"
        "\t_deferredRenderBuf.resize(smplBufferLen);\n",
        "PlayerA deferred buffer sizing",
    )

    c = replace_once(
        c,
        "\t_myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);\n\treturn retVal;\n}\n\nUINT8 PlayerA::Stop(void)\n",
        "\t_myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);\n"
        "\t_deferredEmitSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);\n"
        "\treturn retVal;\n}\n\nUINT8 PlayerA::Stop(void)\n",
        "PlayerA deferred start ordinal",
    )

    c = replace_once(
        c,
        "\tUINT8 retVal = _player->Reset();\n\t_myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);\n\treturn retVal;\n}\n\nUINT8 PlayerA::FadeOut(void)\n",
        "\tUINT8 retVal = _player->Reset();\n"
        "\t_myPlayState = _player->GetState() & (PLAYSTATE_PLAY | PLAYSTATE_END);\n"
        "\t_deferredEmitSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);\n"
        "\treturn retVal;\n}\n\nUINT8 PlayerA::FadeOut(void)\n",
        "PlayerA deferred reset ordinal",
    )

    c = replace_once(
        c,
        "\tif (pbSmpl < _endSilenceStart)\n\t\t_endSilenceStart = (UINT32)-1;\n\treturn retVal;\n}\n\n#if 1\n",
        "\tif (pbSmpl < _endSilenceStart)\n\t\t_endSilenceStart = (UINT32)-1;\n"
        "\t_deferredEmitSmpl = pbSmpl;\n"
        "\treturn retVal;\n}\n\n#if 1\n",
        "PlayerA deferred seek ordinal",
    )

    marker = "UINT8 PlayerA::GetState(void) const\n"
    setter = (
        "void PlayerA::SetDeferredPostRenderProcessor(\n"
        "\tPLR_DEFERRED_POST_RENDER_PROCESSOR cbFunc, void* cbParam)\n"
        "{\n"
        "\tif (_deferredPostRenderFunc == NULL && cbFunc != NULL && _player != NULL)\n"
        "\t\t_deferredEmitSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);\n"
        "\t_deferredPostRenderFunc = cbFunc;\n"
        "\t_deferredPostRenderParam = cbParam;\n"
        "\treturn;\n"
        "}\n\n"
    )
    c = replace_once(c, marker, setter + marker, "PlayerA deferred setter")

    render = r'''UINT32 PlayerA::Render(UINT32 bufSize, void* data)
{
	UINT8* bData = (UINT8*)data;
	UINT32 basePbSmpl;
	UINT32 smplCount;
	UINT32 smplRendered;
	UINT32 curSmpl;
	WAVE_32BS fnlSmpl;	// final sample value
	INT32 curVolume;
	const bool deferred = _deferredPostRenderFunc != NULL;
	
	smplCount = bufSize / _outSmplSizeA;
	if (_player == NULL)
	{
		memset(data, 0x00, smplCount * _outSmplSizeA);
		return smplCount * _outSmplSizeA;
	}
	if (! deferred && ! (_player->GetState() & PLAYSTATE_PLAY))
	{
		memset(data, 0x00, smplCount * _outSmplSizeA);
		return smplCount * _outSmplSizeA;
	}
	
	if (! smplCount)
	{
		_player->Render(0, NULL);	// dummy-rendering
		return 0;
	}
	
	if (smplCount > (UINT32)_smplBuf.size())
		smplCount = (UINT32)_smplBuf.size();
	
	if (! deferred)
	{
		memset(&_smplBuf[0], 0, smplCount * sizeof(WAVE_32BS));
		basePbSmpl = _player->GetCurPos(PLAYPOS_SAMPLE);
		smplRendered = _player->Render(smplCount, &_smplBuf[0]);
		smplCount = smplRendered;
		if (_postRenderFunc != NULL && smplCount != 0)
			_postRenderFunc(_postRenderParam, &_smplBuf[0], smplCount, basePbSmpl);
	}
	else
	{
		// The engine clock may run slightly ahead of the output clock here. The
		// callback owns the protected whole-frame queue; PlayerA owns the emitted
		// ordinal and all downstream gain/fade/packing semantics.
		basePbSmpl = _deferredEmitSmpl;
		UINT32 finalized = 0;
		UINT32 passes = 0;
		memset(&_smplBuf[0], 0, smplCount * sizeof(WAVE_32BS));
		
		while (finalized < smplCount && passes < 16)
		{
			UINT32 produced = 0;
			UINT8 state = _player->GetState();
			UINT8 sourceEnded = ((state & PLAYSTATE_END) || ! (state & PLAYSTATE_PLAY)) ? 1 : 0;
			UINT8 cbResult = _deferredPostRenderFunc(
				_deferredPostRenderParam,
				NULL,
				0,
				_player->GetCurPos(PLAYPOS_SAMPLE),
				&_smplBuf[finalized],
				smplCount - finalized,
				_deferredEmitSmpl + finalized,
				sourceEnded,
				&produced);
			if (cbResult || produced > smplCount - finalized)
				return 0;
			finalized += produced;
			if (finalized >= smplCount)
				break;
			
			state = _player->GetState();
			if (! (state & PLAYSTATE_PLAY))
				break;
			if (_deferredRenderBuf.empty())
				return 0;
			
			UINT32 renderRequest = smplCount;
			if (renderRequest > (UINT32)_deferredRenderBuf.size())
				renderRequest = (UINT32)_deferredRenderBuf.size();
			memset(&_deferredRenderBuf[0], 0, renderRequest * sizeof(WAVE_32BS));
			UINT32 renderedBase = _player->GetCurPos(PLAYPOS_SAMPLE);
			smplRendered = _player->Render(renderRequest, &_deferredRenderBuf[0]);
			if (_postRenderFunc != NULL && smplRendered != 0)
				_postRenderFunc(
					_postRenderParam,
					&_deferredRenderBuf[0],
					smplRendered,
					renderedBase);
			
			state = _player->GetState();
			sourceEnded = ((state & PLAYSTATE_END) || ! (state & PLAYSTATE_PLAY)) ? 1 : 0;
			produced = 0;
			cbResult = _deferredPostRenderFunc(
				_deferredPostRenderParam,
				smplRendered ? &_deferredRenderBuf[0] : NULL,
				smplRendered,
				renderedBase,
				&_smplBuf[finalized],
				smplCount - finalized,
				_deferredEmitSmpl + finalized,
				sourceEnded,
				&produced);
			if (cbResult || produced > smplCount - finalized)
				return 0;
			finalized += produced;
			++passes;
			if (smplRendered == 0 && produced == 0)
				break;
		}
		smplCount = finalized;
		if (! smplCount)
			return 0;
	}
	
	curVolume = CalcCurrentVolume(basePbSmpl) >> VOL_SHIFT;
	for (curSmpl = 0; curSmpl < smplCount; curSmpl ++, basePbSmpl ++)
	{
		if (basePbSmpl >= _fadeSmplStart)
		{
			UINT32 fadeSmpls = basePbSmpl - _fadeSmplStart;
			if (fadeSmpls >= _config.fadeSmpls && ! (_myPlayState & PLAYSTATE_END))
			{
				if (_endSilenceStart == (UINT32)-1)
					_endSilenceStart = basePbSmpl;
				_myPlayState |= PLAYSTATE_END;
			}
			
			curVolume = CalcCurrentVolume(basePbSmpl) >> VOL_SHIFT;
		}
		if (basePbSmpl >= _endSilenceStart)
		{
			UINT32 silenceSmpls = basePbSmpl - _endSilenceStart;
			if (silenceSmpls >= _config.endSilenceSmpls && ! (_myPlayState & PLAYSTATE_FIN))
			{
				_myPlayState |= PLAYSTATE_FIN;
				if (_plrCbFunc != NULL)
					_plrCbFunc(_player, _plrCbParam, PLREVT_END, NULL);
				break;
			}
		}
		
		fnlSmpl = _smplBuf[curSmpl];
		
#ifdef VOLCALC64
		fnlSmpl.L = (INT32)( ((INT64)fnlSmpl.L * curVolume) >> VOL_BITS );
		fnlSmpl.R = (INT32)( ((INT64)fnlSmpl.R * curVolume) >> VOL_BITS );
#else
		fnlSmpl.L = ((fnlSmpl.L >> VOL_PRESH) * curVolume) >> VOL_POSTSH;
		fnlSmpl.R = ((fnlSmpl.R >> VOL_PRESH) * curVolume) >> VOL_POSTSH;
#endif
		
		if (_config.chnInvert & 0x01)
			fnlSmpl.L = -fnlSmpl.L;
		if (_config.chnInvert & 0x02)
			fnlSmpl.R = -fnlSmpl.R;
		
		_outSmplPack(&bData[(curSmpl * 2 + 0) * _outSmplSize1], fnlSmpl.L);
		_outSmplPack(&bData[(curSmpl * 2 + 1) * _outSmplSize1], fnlSmpl.R);
	}
	
	if (deferred)
		_deferredEmitSmpl += curSmpl;
	return curSmpl * _outSmplSizeA;
}

'''
    c = replace_between_once(
        c,
        "UINT32 PlayerA::Render(UINT32 bufSize, void* data)\n",
        "/*static*/ UINT8 PlayerA::PlayCallbackS",
        render,
        "PlayerA deferred render path",
    )

    hpp.write_text(h, encoding="utf-8", newline="\n")
    cpp.write_text(c, encoding="utf-8", newline="\n")
    print("patched PlayerA deferred pre-volume output processor")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
