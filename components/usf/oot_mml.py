"""Ocarina of Time Zelda64 MML semantic surface.

This module does not parse USF containers and does not emulate the N64 audio
runtime.  It describes the authored-program semantics recovered by the
zeldaret/oot decompilation so a future USF runtime trace or ROM sequence
extractor can project exact observations into the project's evidence model.

The critical boundary is intentional:

    USF ROM/save reconstruction != decoded OoT sequence
    decoded OoT sequence != one exact runtime path
    static control flow != exact performed form when state-dependent branches,
                           randomization, dynamic calls, or self-modification
                           can change execution

Source model: zeldaret/oot include/audio/aseq.h and
src/audio/internal/seqplayer.c, MML_VERSION_OOT.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


class OotMmlScope(str, Enum):
    SEQUENCE = "sequence"
    CHANNEL = "channel"
    LAYER = "layer"


class OotMmlEffect(str, Enum):
    NOTE = "note"
    TIMING = "timing"
    CONTROL_FLOW = "control_flow"
    TOPOLOGY = "topology"
    DATA_IO = "data_io"
    MEMORY = "memory"
    RANDOMIZATION = "randomization"
    ALLOCATION = "allocation"
    TEMPO = "tempo"
    PITCH = "pitch"
    DYNAMICS = "dynamics"
    TIMBRE = "timbre"
    ARTICULATION = "articulation"
    MODULATION = "modulation"
    EFFECT = "effect"
    MIX_PARAMETER = "mix_parameter"
    UNKNOWN = "unknown"


class OotStaticRisk(str, Enum):
    NONE = "none"
    STATE_DEPENDENT = "state_dependent"
    DYNAMIC_TARGET = "dynamic_target"
    RANDOMIZED = "randomized"
    SELF_MODIFYING = "self_modifying"


@dataclass(frozen=True)
class OotMmlExecutionContract:
    max_channels: int = 16
    max_layers_per_channel: int = 4
    max_call_depth: int = 4
    channel_execution_is_parallel: bool = True
    layer_execution_is_parallel: bool = True
    local_delays_block_only_local_script: bool = True
    sequence_memory_can_be_modified: bool = True


OOT_MML_EXECUTION_CONTRACT = OotMmlExecutionContract()


@dataclass(frozen=True)
class OotOpcodeSemantic:
    scope: OotMmlScope
    opcode: int
    name: str
    effect: OotMmlEffect
    static_risk: OotStaticRisk = OotStaticRisk.NONE
    encoded_argument: int | None = None
    starts_parallel_script: bool = False
    stops_parallel_script: bool = False
    writes_sequence_memory: bool = False
    description: str = ""

    @property
    def static_execution_is_path_complete(self) -> bool:
        return self.static_risk is OotStaticRisk.NONE


@dataclass(frozen=True)
class _Pattern:
    scope: OotMmlScope
    base: int
    mask: int
    name: str
    effect: OotMmlEffect
    argument_mask: int = 0
    static_risk: OotStaticRisk = OotStaticRisk.NONE
    starts_parallel_script: bool = False
    stops_parallel_script: bool = False
    writes_sequence_memory: bool = False
    description: str = ""

    def matches(self, opcode: int) -> bool:
        return opcode & self.mask == self.base

    def materialize(self, opcode: int) -> OotOpcodeSemantic:
        encoded_argument = opcode & self.argument_mask if self.argument_mask else None
        return OotOpcodeSemantic(
            scope=self.scope,
            opcode=opcode,
            name=self.name,
            effect=self.effect,
            static_risk=self.static_risk,
            encoded_argument=encoded_argument,
            starts_parallel_script=self.starts_parallel_script,
            stops_parallel_script=self.stops_parallel_script,
            writes_sequence_memory=self.writes_sequence_memory,
            description=self.description,
        )


def _p(
    scope: OotMmlScope,
    opcode: int,
    name: str,
    effect: OotMmlEffect,
    *,
    static_risk: OotStaticRisk = OotStaticRisk.NONE,
    starts_parallel_script: bool = False,
    stops_parallel_script: bool = False,
    writes_sequence_memory: bool = False,
    description: str = "",
) -> _Pattern:
    return _Pattern(
        scope=scope,
        base=opcode,
        mask=0xFF,
        name=name,
        effect=effect,
        static_risk=static_risk,
        starts_parallel_script=starts_parallel_script,
        stops_parallel_script=stops_parallel_script,
        writes_sequence_memory=writes_sequence_memory,
        description=description,
    )


def _family(
    scope: OotMmlScope,
    base: int,
    mask: int,
    argument_mask: int,
    name: str,
    effect: OotMmlEffect,
    *,
    static_risk: OotStaticRisk = OotStaticRisk.NONE,
    starts_parallel_script: bool = False,
    stops_parallel_script: bool = False,
    writes_sequence_memory: bool = False,
    description: str = "",
) -> _Pattern:
    return _Pattern(
        scope=scope,
        base=base,
        mask=mask,
        argument_mask=argument_mask,
        name=name,
        effect=effect,
        static_risk=static_risk,
        starts_parallel_script=starts_parallel_script,
        stops_parallel_script=stops_parallel_script,
        writes_sequence_memory=writes_sequence_memory,
        description=description,
    )


# Common flow opcodes are shared by sequence, channel and layer scripts.
_COMMON_FLOW: tuple[_Pattern, ...] = (
    _p(OotMmlScope.SEQUENCE, 0xF2, "RBLTZ", OotMmlEffect.CONTROL_FLOW, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _p(OotMmlScope.SEQUENCE, 0xF3, "RBEQZ", OotMmlEffect.CONTROL_FLOW, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _p(OotMmlScope.SEQUENCE, 0xF4, "RJUMP", OotMmlEffect.CONTROL_FLOW),
    _p(OotMmlScope.SEQUENCE, 0xF5, "BGEZ", OotMmlEffect.CONTROL_FLOW, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _p(OotMmlScope.SEQUENCE, 0xF6, "BREAK", OotMmlEffect.CONTROL_FLOW),
    _p(OotMmlScope.SEQUENCE, 0xF7, "LOOPEND", OotMmlEffect.CONTROL_FLOW),
    _p(OotMmlScope.SEQUENCE, 0xF8, "LOOP", OotMmlEffect.CONTROL_FLOW),
    _p(OotMmlScope.SEQUENCE, 0xF9, "BLTZ", OotMmlEffect.CONTROL_FLOW, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _p(OotMmlScope.SEQUENCE, 0xFA, "BEQZ", OotMmlEffect.CONTROL_FLOW, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _p(OotMmlScope.SEQUENCE, 0xFB, "JUMP", OotMmlEffect.CONTROL_FLOW),
    _p(OotMmlScope.SEQUENCE, 0xFC, "CALL", OotMmlEffect.CONTROL_FLOW),
    _p(OotMmlScope.SEQUENCE, 0xFD, "DELAY", OotMmlEffect.TIMING),
    _p(OotMmlScope.SEQUENCE, 0xFE, "DELAY1", OotMmlEffect.TIMING),
    _p(OotMmlScope.SEQUENCE, 0xFF, "END", OotMmlEffect.CONTROL_FLOW),
)


_SEQUENCE_PATTERNS: tuple[_Pattern, ...] = (
    _family(OotMmlScope.SEQUENCE, 0x00, 0xF0, 0x0F, "TESTCHAN", OotMmlEffect.TOPOLOGY, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _family(OotMmlScope.SEQUENCE, 0x40, 0xF0, 0x0F, "STOPCHAN", OotMmlEffect.TOPOLOGY, stops_parallel_script=True),
    _family(OotMmlScope.SEQUENCE, 0x50, 0xF0, 0x0F, "SUBIO", OotMmlEffect.DATA_IO),
    _family(OotMmlScope.SEQUENCE, 0x60, 0xF0, 0x0F, "LDRES", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _family(OotMmlScope.SEQUENCE, 0x70, 0xF0, 0x0F, "STIO", OotMmlEffect.DATA_IO),
    _family(OotMmlScope.SEQUENCE, 0x80, 0xF0, 0x0F, "LDIO", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _family(OotMmlScope.SEQUENCE, 0x90, 0xF0, 0x0F, "LDCHAN", OotMmlEffect.TOPOLOGY, starts_parallel_script=True),
    _family(OotMmlScope.SEQUENCE, 0xA0, 0xF0, 0x0F, "RLDCHAN", OotMmlEffect.TOPOLOGY, starts_parallel_script=True),
    _family(OotMmlScope.SEQUENCE, 0xB0, 0xF0, 0x0F, "LDSEQ", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.SEQUENCE, 0xC4, "RUNSEQ", OotMmlEffect.TOPOLOGY, starts_parallel_script=True),
    _p(OotMmlScope.SEQUENCE, 0xC5, "SCRIPTCTR", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _p(OotMmlScope.SEQUENCE, 0xC6, "STOP", OotMmlEffect.CONTROL_FLOW),
    _p(OotMmlScope.SEQUENCE, 0xC7, "STSEQ", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.SELF_MODIFYING, writes_sequence_memory=True),
    _p(OotMmlScope.SEQUENCE, 0xC8, "SUB", OotMmlEffect.DATA_IO),
    _p(OotMmlScope.SEQUENCE, 0xC9, "AND", OotMmlEffect.DATA_IO),
    _p(OotMmlScope.SEQUENCE, 0xCC, "LDI", OotMmlEffect.DATA_IO),
    _p(OotMmlScope.SEQUENCE, 0xCD, "DYNCALL", OotMmlEffect.CONTROL_FLOW, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.SEQUENCE, 0xCE, "RAND", OotMmlEffect.RANDOMIZATION, static_risk=OotStaticRisk.RANDOMIZED),
    _p(OotMmlScope.SEQUENCE, 0xD0, "NOTEALLOC", OotMmlEffect.ALLOCATION),
    _p(OotMmlScope.SEQUENCE, 0xD1, "LDSHORTGATEARR", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.SEQUENCE, 0xD2, "LDSHORTVELARR", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.SEQUENCE, 0xD3, "MUTEBHV", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.SEQUENCE, 0xD4, "MUTE", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.SEQUENCE, 0xD5, "MUTESCALE", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.SEQUENCE, 0xD6, "FREECHAN", OotMmlEffect.TOPOLOGY, stops_parallel_script=True),
    _p(OotMmlScope.SEQUENCE, 0xD7, "INITCHAN", OotMmlEffect.TOPOLOGY, starts_parallel_script=True),
    _p(OotMmlScope.SEQUENCE, 0xD9, "VOLSCALE", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.SEQUENCE, 0xDA, "VOLMODE", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.SEQUENCE, 0xDB, "VOL", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.SEQUENCE, 0xDC, "TEMPOCHG", OotMmlEffect.TEMPO),
    _p(OotMmlScope.SEQUENCE, 0xDD, "TEMPO", OotMmlEffect.TEMPO),
    _p(OotMmlScope.SEQUENCE, 0xDE, "RTRANSPOSE", OotMmlEffect.PITCH),
    _p(OotMmlScope.SEQUENCE, 0xDF, "TRANSPOSE", OotMmlEffect.PITCH),
    _p(OotMmlScope.SEQUENCE, 0xF0, "FREENOTELIST", OotMmlEffect.ALLOCATION),
    _p(OotMmlScope.SEQUENCE, 0xF1, "ALLOCNOTELIST", OotMmlEffect.ALLOCATION),
)


_CHANNEL_PATTERNS: tuple[_Pattern, ...] = (
    _family(OotMmlScope.CHANNEL, 0x00, 0xF0, 0x0F, "CDELAY", OotMmlEffect.TIMING),
    _family(OotMmlScope.CHANNEL, 0x10, 0xF0, 0x0F, "LDSAMPLE", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _family(OotMmlScope.CHANNEL, 0x20, 0xF0, 0x0F, "LDCHAN", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _family(OotMmlScope.CHANNEL, 0x30, 0xF0, 0x0F, "STCIO", OotMmlEffect.DATA_IO),
    _family(OotMmlScope.CHANNEL, 0x40, 0xF0, 0x0F, "LDCIO", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _family(OotMmlScope.CHANNEL, 0x50, 0xF0, 0x0F, "SUBIO", OotMmlEffect.DATA_IO),
    _family(OotMmlScope.CHANNEL, 0x60, 0xF0, 0x0F, "LDIO", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _family(OotMmlScope.CHANNEL, 0x70, 0xF8, 0x07, "STIO", OotMmlEffect.DATA_IO),
    _family(OotMmlScope.CHANNEL, 0x78, 0xF8, 0x07, "RLDLAYER", OotMmlEffect.TOPOLOGY, starts_parallel_script=True),
    _family(OotMmlScope.CHANNEL, 0x80, 0xF8, 0x07, "TESTLAYER", OotMmlEffect.TOPOLOGY, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _family(OotMmlScope.CHANNEL, 0x88, 0xF8, 0x07, "LDLAYER", OotMmlEffect.TOPOLOGY, starts_parallel_script=True),
    _family(OotMmlScope.CHANNEL, 0x90, 0xF8, 0x07, "DELLAYER", OotMmlEffect.TOPOLOGY, stops_parallel_script=True),
    _family(OotMmlScope.CHANNEL, 0x98, 0xF8, 0x07, "DYNLDLAYER", OotMmlEffect.TOPOLOGY, static_risk=OotStaticRisk.DYNAMIC_TARGET, starts_parallel_script=True),
    _p(OotMmlScope.CHANNEL, 0xB0, "LDFILTER", OotMmlEffect.EFFECT),
    _p(OotMmlScope.CHANNEL, 0xB1, "FREEFILTER", OotMmlEffect.EFFECT),
    _p(OotMmlScope.CHANNEL, 0xB2, "LDSEQTOPTR", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.CHANNEL, 0xB3, "FILTER", OotMmlEffect.EFFECT),
    _p(OotMmlScope.CHANNEL, 0xB4, "PTRTODYNTBL", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.CHANNEL, 0xB5, "DYNTBLTOPTR", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.CHANNEL, 0xB6, "DYNTBLV", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _p(OotMmlScope.CHANNEL, 0xB7, "RANDTOPTR", OotMmlEffect.RANDOMIZATION, static_risk=OotStaticRisk.RANDOMIZED),
    _p(OotMmlScope.CHANNEL, 0xB8, "RAND", OotMmlEffect.RANDOMIZATION, static_risk=OotStaticRisk.RANDOMIZED),
    _p(OotMmlScope.CHANNEL, 0xB9, "RANDVEL", OotMmlEffect.RANDOMIZATION, static_risk=OotStaticRisk.RANDOMIZED),
    _p(OotMmlScope.CHANNEL, 0xBA, "RANDGATE", OotMmlEffect.RANDOMIZATION, static_risk=OotStaticRisk.RANDOMIZED),
    _p(OotMmlScope.CHANNEL, 0xBB, "COMBFILTER", OotMmlEffect.EFFECT),
    _p(OotMmlScope.CHANNEL, 0xBC, "PTRADD", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.CHANNEL, 0xBD, "RANDPTR", OotMmlEffect.RANDOMIZATION, static_risk=OotStaticRisk.RANDOMIZED),
    _p(OotMmlScope.CHANNEL, 0xC1, "INSTR", OotMmlEffect.TIMBRE),
    _p(OotMmlScope.CHANNEL, 0xC2, "DYNTBL", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.CHANNEL, 0xC3, "SHORT", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.CHANNEL, 0xC4, "NOSHORT", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.CHANNEL, 0xC5, "DYNTBLLOOKUP", OotMmlEffect.DATA_IO, static_risk=OotStaticRisk.STATE_DEPENDENT),
    _p(OotMmlScope.CHANNEL, 0xC6, "FONT", OotMmlEffect.TIMBRE),
    _p(OotMmlScope.CHANNEL, 0xC7, "STSEQ", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.SELF_MODIFYING, writes_sequence_memory=True),
    _p(OotMmlScope.CHANNEL, 0xC8, "SUB", OotMmlEffect.DATA_IO),
    _p(OotMmlScope.CHANNEL, 0xC9, "AND", OotMmlEffect.DATA_IO),
    _p(OotMmlScope.CHANNEL, 0xCA, "MUTEBHV", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.CHANNEL, 0xCB, "LDSEQ", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.CHANNEL, 0xCC, "LDI", OotMmlEffect.DATA_IO),
    _p(OotMmlScope.CHANNEL, 0xCD, "STOPCHAN", OotMmlEffect.TOPOLOGY, stops_parallel_script=True),
    _p(OotMmlScope.CHANNEL, 0xCE, "LDPTR", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.CHANNEL, 0xCF, "STPTRTOSEQ", OotMmlEffect.MEMORY, static_risk=OotStaticRisk.SELF_MODIFYING, writes_sequence_memory=True),
    _p(OotMmlScope.CHANNEL, 0xD0, "EFFECTS", OotMmlEffect.EFFECT),
    _p(OotMmlScope.CHANNEL, 0xD1, "NOTEALLOC", OotMmlEffect.ALLOCATION),
    _p(OotMmlScope.CHANNEL, 0xD2, "SUSTAIN", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.CHANNEL, 0xD3, "BEND", OotMmlEffect.PITCH),
    _p(OotMmlScope.CHANNEL, 0xD4, "REVERB", OotMmlEffect.EFFECT),
    _p(OotMmlScope.CHANNEL, 0xD7, "VIBFREQ", OotMmlEffect.MODULATION),
    _p(OotMmlScope.CHANNEL, 0xD8, "VIBDEPTH", OotMmlEffect.MODULATION),
    _p(OotMmlScope.CHANNEL, 0xD9, "RELEASERATE", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.CHANNEL, 0xDA, "ENV", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.CHANNEL, 0xDB, "TRANSPOSE", OotMmlEffect.PITCH),
    # Pan/stereo controls are preserved as authored mix parameters only. Spatial
    # interpretation belongs to the separate spatial-audio workstream.
    _p(OotMmlScope.CHANNEL, 0xDC, "PANWEIGHT", OotMmlEffect.MIX_PARAMETER),
    _p(OotMmlScope.CHANNEL, 0xDD, "PAN", OotMmlEffect.MIX_PARAMETER),
    _p(OotMmlScope.CHANNEL, 0xDE, "FREQSCALE", OotMmlEffect.PITCH),
    _p(OotMmlScope.CHANNEL, 0xDF, "VOL", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.CHANNEL, 0xE0, "VOLEXP", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.CHANNEL, 0xE1, "VIBFREQGRAD", OotMmlEffect.MODULATION),
    _p(OotMmlScope.CHANNEL, 0xE2, "VIBDEPTHGRAD", OotMmlEffect.MODULATION),
    _p(OotMmlScope.CHANNEL, 0xE3, "VIBDELAY", OotMmlEffect.MODULATION),
    _p(OotMmlScope.CHANNEL, 0xE4, "DYNCALL", OotMmlEffect.CONTROL_FLOW, static_risk=OotStaticRisk.DYNAMIC_TARGET),
    _p(OotMmlScope.CHANNEL, 0xE5, "REVERBIDX", OotMmlEffect.EFFECT),
    _p(OotMmlScope.CHANNEL, 0xE6, "SAMPLEBOOK", OotMmlEffect.TIMBRE),
    _p(OotMmlScope.CHANNEL, 0xE7, "LDPARAMS", OotMmlEffect.DATA_IO),
    _p(OotMmlScope.CHANNEL, 0xE8, "PARAMS", OotMmlEffect.DATA_IO),
    _p(OotMmlScope.CHANNEL, 0xE9, "NOTEPRI", OotMmlEffect.ALLOCATION),
    _p(OotMmlScope.CHANNEL, 0xEA, "STOP", OotMmlEffect.CONTROL_FLOW),
    _p(OotMmlScope.CHANNEL, 0xEB, "FONTINSTR", OotMmlEffect.TIMBRE),
    _p(OotMmlScope.CHANNEL, 0xEC, "VIBRESET", OotMmlEffect.MODULATION),
    _p(OotMmlScope.CHANNEL, 0xED, "GAIN", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.CHANNEL, 0xEE, "BENDFINE", OotMmlEffect.PITCH),
    _p(OotMmlScope.CHANNEL, 0xF0, "FREENOTELIST", OotMmlEffect.ALLOCATION),
    _p(OotMmlScope.CHANNEL, 0xF1, "ALLOCNOTELIST", OotMmlEffect.ALLOCATION),
)


_LAYER_PATTERNS: tuple[_Pattern, ...] = (
    # In each note family the low six bits encode pitch.
    _family(OotMmlScope.LAYER, 0x00, 0xC0, 0x3F, "NOTEDVG", OotMmlEffect.NOTE),
    _family(OotMmlScope.LAYER, 0x40, 0xC0, 0x3F, "NOTEDV", OotMmlEffect.NOTE),
    _family(OotMmlScope.LAYER, 0x80, 0xC0, 0x3F, "NOTEVG", OotMmlEffect.NOTE),
    _p(OotMmlScope.LAYER, 0xC0, "LDELAY", OotMmlEffect.TIMING),
    _p(OotMmlScope.LAYER, 0xC1, "SHORTVEL", OotMmlEffect.DYNAMICS),
    _p(OotMmlScope.LAYER, 0xC2, "TRANSPOSE", OotMmlEffect.PITCH),
    _p(OotMmlScope.LAYER, 0xC3, "SHORTDELAY", OotMmlEffect.TIMING),
    _p(OotMmlScope.LAYER, 0xC4, "LEGATO", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.LAYER, 0xC5, "NOLEGATO", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.LAYER, 0xC6, "INSTR", OotMmlEffect.TIMBRE),
    _p(OotMmlScope.LAYER, 0xC7, "PORTAMENTO", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.LAYER, 0xC8, "NOPORTAMENTO", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.LAYER, 0xC9, "SHORTGATE", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.LAYER, 0xCA, "NOTEPAN", OotMmlEffect.MIX_PARAMETER),
    _p(OotMmlScope.LAYER, 0xCB, "ENV", OotMmlEffect.ARTICULATION),
    _p(OotMmlScope.LAYER, 0xCC, "NODRUMPAN", OotMmlEffect.MIX_PARAMETER),
    _p(OotMmlScope.LAYER, 0xCD, "STEREO", OotMmlEffect.MIX_PARAMETER),
    _p(OotMmlScope.LAYER, 0xCE, "BENDFINE", OotMmlEffect.PITCH),
    _p(OotMmlScope.LAYER, 0xCF, "RELEASERATE", OotMmlEffect.ARTICULATION),
    _family(OotMmlScope.LAYER, 0xD0, 0xF0, 0x0F, "LDSHORTVEL", OotMmlEffect.DYNAMICS),
    _family(OotMmlScope.LAYER, 0xE0, 0xF0, 0x0F, "LDSHORTGATE", OotMmlEffect.ARTICULATION),
)


def _common_for_scope(scope: OotMmlScope) -> tuple[_Pattern, ...]:
    # Re-materialize the common instruction set at the requested script scope.
    return tuple(
        _Pattern(
            scope=scope,
            base=item.base,
            mask=item.mask,
            name=item.name,
            effect=item.effect,
            argument_mask=item.argument_mask,
            static_risk=item.static_risk,
            starts_parallel_script=item.starts_parallel_script,
            stops_parallel_script=item.stops_parallel_script,
            writes_sequence_memory=item.writes_sequence_memory,
            description=item.description,
        )
        for item in _COMMON_FLOW
    )


def classify_oot_opcode(scope: OotMmlScope | str, opcode: int) -> OotOpcodeSemantic:
    """Classify one OoT MML opcode without pretending to decode its arguments.

    This is deliberately a semantic surface, not a byte-stream parser. Variable
    argument widths, compressed delays, script-relative targets, dynamic tables,
    and runtime state belong in the later authored-program decoder/runtime bridge.
    """

    try:
        scope = OotMmlScope(scope)
    except ValueError as exc:
        raise ValueError(f"unknown OoT MML scope: {scope}") from exc
    if opcode < 0 or opcode > 0xFF:
        raise ValueError("OoT MML opcode must be in [0, 255]")

    if opcode >= 0xF2:
        patterns = _common_for_scope(scope)
    elif scope is OotMmlScope.SEQUENCE:
        patterns = _SEQUENCE_PATTERNS
    elif scope is OotMmlScope.CHANNEL:
        patterns = _CHANNEL_PATTERNS
    else:
        patterns = _LAYER_PATTERNS

    for pattern in patterns:
        if pattern.matches(opcode):
            return pattern.materialize(opcode)

    return OotOpcodeSemantic(
        scope=scope,
        opcode=opcode,
        name=f"UNKNOWN_{opcode:02X}",
        effect=OotMmlEffect.UNKNOWN,
        static_risk=OotStaticRisk.STATE_DEPENDENT,
        description="opcode semantics not established by this OoT semantic table",
    )


def authored_path_requires_runtime_confirmation(
    semantics: tuple[OotOpcodeSemantic, ...] | list[OotOpcodeSemantic],
) -> bool:
    """Return whether an authored path contains execution-ambiguating behavior."""

    return any(not item.static_execution_is_path_complete for item in semantics)


def musical_effects(
    semantics: tuple[OotOpcodeSemantic, ...] | list[OotOpcodeSemantic],
) -> frozenset[OotMmlEffect]:
    """Project opcode semantics onto non-spatial musical analysis dimensions."""

    return frozenset(
        item.effect
        for item in semantics
        if item.effect
        not in {
            OotMmlEffect.UNKNOWN,
            OotMmlEffect.DATA_IO,
            OotMmlEffect.MEMORY,
            OotMmlEffect.CONTROL_FLOW,
            OotMmlEffect.MIX_PARAMETER,
        }
    )
