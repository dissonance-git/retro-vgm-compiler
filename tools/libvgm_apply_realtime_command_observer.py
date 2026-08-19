#!/usr/bin/env python3
"""Install the command-observer ABI on the pinned libvgm tree with exact anchors."""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("libvgm_root", type=Path)
    root = parser.parse_args().libvgm_root.resolve()
    hpp = root / "player" / "vgmplayer.hpp"
    cpp = root / "player" / "vgmplayer.cpp"

    hpp_text = hpp.read_text(encoding="utf-8")
    cpp_text = cpp.read_text(encoding="utf-8")
    if "LIBVGM_GAMEAUDIO_COMMAND_OBSERVER" in hpp_text:
        required_hpp = (
            "VGM_COMMAND_OBSERVER_EVENT",
            "SetCommandObserver",
            "NotifyCommandObserver(UINT8 command)",
            "NotifyCommandObserverReset(void)",
            "_commandObserverParam",
        )
        required_cpp = (
            "void VGMPlayer::SetCommandObserver",
            "void VGMPlayer::NotifyCommandObserver(UINT8 command)",
            "void VGMPlayer::NotifyCommandObserverReset(void)",
            "NotifyCommandObserver(curCmd);",
            "NotifyCommandObserverReset();",
        )
        missing = [x for x in required_hpp if x not in hpp_text] + [x for x in required_cpp if x not in cpp_text]
        if missing:
            raise RuntimeError(f"partial command-observer ABI already present: {missing}")
        print("libvgm realtime command observer already installed")
        return 0

    replace_once(
        hpp,
        """\tUINT8 hardStopOld;\t// enforce silence at end of old VGMs (<1.50), fixes Key Off events being trimmed off\n};\n\n\nclass VGMPlayer : public PlayerBase\n""",
        """\tUINT8 hardStopOld;\t// enforce silence at end of old VGMs (<1.50), fixes Key Off events being trimmed off\n};\n\n#define LIBVGM_GAMEAUDIO_COMMAND_OBSERVER 1\n\nenum VGM_COMMAND_OBSERVER_EVENT_TYPE\n{\n\tVGMCOE_RESET = 0,\n\tVGMCOE_COMMAND = 1,\n};\n\nstruct VGM_COMMAND_OBSERVER_EVENT\n{\n\tUINT8 type;\n\tUINT32 tick;\n\tUINT32 filePos;\n\tUINT8 command;\n\tconst UINT8* payload;\n\tUINT32 payloadLen;\n};\n\ntypedef void (*VGM_COMMAND_OBSERVER_CB)(void* userParam,\n\t\tconst VGM_COMMAND_OBSERVER_EVENT* event);\n\n\nclass VGMPlayer : public PlayerBase\n""",
        "command observer public ABI",
    )
    replace_once(
        hpp,
        """\tUINT32 GetModifiedLoopCount(UINT32 defaultLoops) const;\t// get loop count, modified according to LoopModified/LoopBase header\n\tconst std::vector<DACSTRM_DEV>& GetStreamDevInfo(void) const;\n\t\n\tUINT8 Start(void);\n""",
        """\tUINT32 GetModifiedLoopCount(UINT32 defaultLoops) const;\t// get loop count, modified according to LoopModified/LoopBase header\n\tconst std::vector<DACSTRM_DEV>& GetStreamDevInfo(void) const;\n\tvoid SetCommandObserver(VGM_COMMAND_OBSERVER_CB cbFunc, void* cbParam);\n\t\n\tUINT8 Start(void);\n""",
        "command observer setter declaration",
    )
    replace_once(
        hpp,
        """\tUINT8 SeekToTick(UINT32 tick);\n\tUINT8 SeekToFilePos(UINT32 pos);\n\tvoid ParseFile(UINT32 ticks);\n\n\tvoid ParseFileForFMClocks();\n""",
        """\tUINT8 SeekToTick(UINT32 tick);\n\tUINT8 SeekToFilePos(UINT32 pos);\n\tvoid ParseFile(UINT32 ticks);\n\tvoid NotifyCommandObserver(UINT8 command);\n\tvoid NotifyCommandObserverReset(void);\n\n\tvoid ParseFileForFMClocks();\n""",
        "command observer notification declarations",
    )
    replace_once(
        hpp,
        """\tUINT8 _playState;\n\tUINT8 _psTrigger;\t// used to temporarily trigger special commands\n\t//PLAYER_EVENT_CB _eventCbFunc;\n""",
        """\tUINT8 _playState;\n\tUINT8 _psTrigger;\t// used to temporarily trigger special commands\n\tVGM_COMMAND_OBSERVER_CB _commandObserver;\n\tvoid* _commandObserverParam;\n\t//PLAYER_EVENT_CB _eventCbFunc;\n""",
        "command observer state",
    )

    replace_once(
        cpp,
        """\t_curLoop(0),\n\t_playState(0x00),\n\t_psTrigger(0x00)\n{\n""",
        """\t_curLoop(0),\n\t_playState(0x00),\n\t_psTrigger(0x00),\n\t_commandObserver(NULL),\n\t_commandObserverParam(NULL)\n{\n""",
        "command observer constructor state",
    )
    replace_once(
        cpp,
        """VGMPlayer::~VGMPlayer()\n{\n\t_eventCbFunc = NULL;\t// prevent any callbacks during destruction\n\t\n""",
        """VGMPlayer::~VGMPlayer()\n{\n\t_eventCbFunc = NULL;\t// prevent any callbacks during destruction\n\t_commandObserver = NULL;\n\t_commandObserverParam = NULL;\n\t\n""",
        "command observer destructor state",
    )
    replace_once(
        cpp,
        """const std::vector<VGMPlayer::DACSTRM_DEV>& VGMPlayer::GetStreamDevInfo(void) const\n{\n\treturn _dacStreams;\n}\n\n/*static*/ void VGMPlayer::PlayerLogCB""",
        """const std::vector<VGMPlayer::DACSTRM_DEV>& VGMPlayer::GetStreamDevInfo(void) const\n{\n\treturn _dacStreams;\n}\n\nvoid VGMPlayer::SetCommandObserver(VGM_COMMAND_OBSERVER_CB cbFunc, void* cbParam)\n{\n\t_commandObserver = cbFunc;\n\t_commandObserverParam = cbParam;\n\treturn;\n}\n\nvoid VGMPlayer::NotifyCommandObserver(UINT8 command)\n{\n\tif (_commandObserver == NULL)\n\t\treturn;\n\n\tUINT32 cmdLen = _CMD_INFO[command].cmdLen;\n\tUINT32 payloadLen = 0;\n\tconst UINT8* payload = NULL;\n\tif (cmdLen > 1 && _filePos < _fileHdr.dataEnd)\n\t{\n\t\tUINT32 bytesLeft = _fileHdr.dataEnd - _filePos;\n\t\tif (cmdLen <= bytesLeft)\n\t\t{\n\t\t\tpayload = &_fileData[_filePos + 1];\n\t\t\tpayloadLen = cmdLen - 1;\n\t\t}\n\t}\n\n\tVGM_COMMAND_OBSERVER_EVENT event;\n\tevent.type = VGMCOE_COMMAND;\n\tevent.tick = _fileTick;\n\tevent.filePos = _filePos;\n\tevent.command = command;\n\tevent.payload = payload;\n\tevent.payloadLen = payloadLen;\n\t_commandObserver(_commandObserverParam, &event);\n\treturn;\n}\n\nvoid VGMPlayer::NotifyCommandObserverReset(void)\n{\n\tif (_commandObserver == NULL)\n\t\treturn;\n\tVGM_COMMAND_OBSERVER_EVENT event = {VGMCOE_RESET, 0, _filePos, 0, NULL, 0};\n\t_commandObserver(_commandObserverParam, &event);\n\treturn;\n}\n\n/*static*/ void VGMPlayer::PlayerLogCB""",
        "command observer implementations",
    )
    replace_once(
        cpp,
        """\t_psTrigger = 0x00;\n\t_curLoop = 0;\n\t_lastLoopTick = 0;\n\t\n\tRefreshTSRates();\n""",
        """\t_psTrigger = 0x00;\n\t_curLoop = 0;\n\t_lastLoopTick = 0;\n\tNotifyCommandObserverReset();\n\t\n\tRefreshTSRates();\n""",
        "command observer reset notification",
    )
    seek_loop = """\twhile(_filePos < _fileHdr.dataEnd && _filePos <= pos && ! (_playState & PLAYSTATE_END))\n\t{\n\t\tUINT8 curCmd = _fileData[_filePos];\n\t\tCOMMAND_FUNC func = _CMD_INFO[curCmd].func;\n"""
    seek_new = """\twhile(_filePos < _fileHdr.dataEnd && _filePos <= pos && ! (_playState & PLAYSTATE_END))\n\t{\n\t\tUINT8 curCmd = _fileData[_filePos];\n\t\tNotifyCommandObserver(curCmd);\n\t\tCOMMAND_FUNC func = _CMD_INFO[curCmd].func;\n"""
    replace_once(cpp, seek_loop, seek_new, "command observer seek notification")
    parse_loop = """\twhile(_filePos < _fileHdr.dataEnd && _fileTick <= _playTick && ! (_playState & PLAYSTATE_END))\n\t{\n\t\tUINT8 curCmd = _fileData[_filePos];\n\t\tCOMMAND_FUNC func = _CMD_INFO[curCmd].func;\n"""
    parse_new = """\twhile(_filePos < _fileHdr.dataEnd && _fileTick <= _playTick && ! (_playState & PLAYSTATE_END))\n\t{\n\t\tUINT8 curCmd = _fileData[_filePos];\n\t\tNotifyCommandObserver(curCmd);\n\t\tCOMMAND_FUNC func = _CMD_INFO[curCmd].func;\n"""
    replace_once(cpp, parse_loop, parse_new, "command observer playback notification")

    print("libvgm realtime command observer installed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
