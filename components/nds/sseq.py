"""Nintendo DS SSEQ authored-program semantics shared by 2SF and NCSF.

This module is intentionally below musical interpretation and above container
reconstruction.  It classifies Nintendo DS sequence commands without claiming
that a static byte walk equals the performed runtime path.

Grounding used for the initial table:
- VGMTrans NDSSeq.cpp
- ndspy soundSequence.py
- pret/pokeplatinum tools/nitrosfx/sseq.c

All three independently expose the same core command surface.  Where names
conflict, this table prefers the runtime/musical meaning established by multiple
implementations (for example C4/C5 are pitch-bend and bend-range controls).
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


class SseqEffect(str, Enum):
    NOTE = "note"
    TIMING = "timing"
    CONTROL_FLOW = "control_flow"
    TRACK_TOPOLOGY = "track_topology"
    VARIABLE_STATE = "variable_state"
    RANDOMIZATION = "randomization"
    TIMBRE = "timbre"
    DYNAMICS = "dynamics"
    PITCH = "pitch"
    ARTICULATION = "articulation"
    MODULATION = "modulation"
    TEMPO = "tempo"
    LOOP = "loop"
    MIX_PARAMETER = "mix_parameter"
    DEBUG = "debug"
    UNKNOWN = "unknown"


class SseqStaticRisk(str, Enum):
    NONE = "none"
    STATE_DEPENDENT = "state_dependent"
    RANDOMIZED = "randomized"
    DYNAMIC_ARGUMENT = "dynamic_argument"


@dataclass(frozen=True)
class SseqExecutionContract:
    max_tracks: int = 16
    notes_encode_pitch_in_opcode: bool = True
    tracks_execute_concurrently: bool = True
    has_variables: bool = True
    has_conditional_execution: bool = True
    has_randomized_arguments: bool = True
    has_calls_and_jumps: bool = True


SSEQ_EXECUTION_CONTRACT = SseqExecutionContract()


@dataclass(frozen=True)
class SseqOpcodeSemantic:
    opcode: int
    name: str
    effect: SseqEffect
    static_risk: SseqStaticRisk = SseqStaticRisk.NONE
    encoded_pitch: int | None = None
    opens_track: bool = False
    ends_track: bool = False
    wraps_subcommand: bool = False
    global_scope: bool = False
    description: str = ""

    @property
    def static_execution_is_path_complete(self) -> bool:
        return self.static_risk is SseqStaticRisk.NONE


_FIXED: dict[int, SseqOpcodeSemantic] = {
    0x80: SseqOpcodeSemantic(0x80, "WAIT", SseqEffect.TIMING),
    0x81: SseqOpcodeSemantic(0x81, "INSTRUMENT", SseqEffect.TIMBRE),
    0x93: SseqOpcodeSemantic(0x93, "OPEN_TRACK", SseqEffect.TRACK_TOPOLOGY, opens_track=True),
    0x94: SseqOpcodeSemantic(0x94, "JUMP", SseqEffect.CONTROL_FLOW),
    0x95: SseqOpcodeSemantic(0x95, "CALL", SseqEffect.CONTROL_FLOW),
    0xA0: SseqOpcodeSemantic(
        0xA0,
        "RANDOM",
        SseqEffect.RANDOMIZATION,
        SseqStaticRisk.RANDOMIZED,
        wraps_subcommand=True,
        description="execute another event with a randomized argument",
    ),
    0xA1: SseqOpcodeSemantic(
        0xA1,
        "FROM_VARIABLE",
        SseqEffect.VARIABLE_STATE,
        SseqStaticRisk.DYNAMIC_ARGUMENT,
        wraps_subcommand=True,
        description="execute another event with an argument sourced from a sequence variable",
    ),
    0xA2: SseqOpcodeSemantic(
        0xA2,
        "IF",
        SseqEffect.CONTROL_FLOW,
        SseqStaticRisk.STATE_DEPENDENT,
        description="skip the following event when the conditional flag is false",
    ),
    0xB0: SseqOpcodeSemantic(0xB0, "SET_VARIABLE", SseqEffect.VARIABLE_STATE),
    0xB1: SseqOpcodeSemantic(0xB1, "ADD_VARIABLE", SseqEffect.VARIABLE_STATE),
    0xB2: SseqOpcodeSemantic(0xB2, "SUB_VARIABLE", SseqEffect.VARIABLE_STATE),
    0xB3: SseqOpcodeSemantic(0xB3, "MUL_VARIABLE", SseqEffect.VARIABLE_STATE),
    0xB4: SseqOpcodeSemantic(0xB4, "DIV_VARIABLE", SseqEffect.VARIABLE_STATE),
    0xB5: SseqOpcodeSemantic(0xB5, "SHIFT_VARIABLE", SseqEffect.VARIABLE_STATE),
    0xB6: SseqOpcodeSemantic(0xB6, "RAND_VARIABLE", SseqEffect.RANDOMIZATION, SseqStaticRisk.RANDOMIZED),
    0xB7: SseqOpcodeSemantic(0xB7, "VARIABLE_NOP", SseqEffect.VARIABLE_STATE),
    0xB8: SseqOpcodeSemantic(0xB8, "CMP_EQ", SseqEffect.VARIABLE_STATE),
    0xB9: SseqOpcodeSemantic(0xB9, "CMP_GE", SseqEffect.VARIABLE_STATE),
    0xBA: SseqOpcodeSemantic(0xBA, "CMP_GT", SseqEffect.VARIABLE_STATE),
    0xBB: SseqOpcodeSemantic(0xBB, "CMP_LE", SseqEffect.VARIABLE_STATE),
    0xBC: SseqOpcodeSemantic(0xBC, "CMP_LT", SseqEffect.VARIABLE_STATE),
    0xBD: SseqOpcodeSemantic(0xBD, "CMP_NE", SseqEffect.VARIABLE_STATE),
    # Preserve authored pan as a mix fact only. Spatial interpretation belongs
    # to the separate spatial-audio workstream.
    0xC0: SseqOpcodeSemantic(0xC0, "PAN", SseqEffect.MIX_PARAMETER),
    0xC1: SseqOpcodeSemantic(0xC1, "TRACK_VOLUME", SseqEffect.DYNAMICS),
    0xC2: SseqOpcodeSemantic(0xC2, "MASTER_VOLUME", SseqEffect.DYNAMICS, global_scope=True),
    0xC3: SseqOpcodeSemantic(0xC3, "TRANSPOSE", SseqEffect.PITCH),
    0xC4: SseqOpcodeSemantic(0xC4, "PITCH_BEND", SseqEffect.PITCH),
    0xC5: SseqOpcodeSemantic(0xC5, "PITCH_BEND_RANGE", SseqEffect.PITCH),
    0xC6: SseqOpcodeSemantic(0xC6, "PRIORITY", SseqEffect.TRACK_TOPOLOGY),
    0xC7: SseqOpcodeSemantic(0xC7, "MONO_POLY", SseqEffect.ARTICULATION),
    0xC8: SseqOpcodeSemantic(0xC8, "TIE", SseqEffect.ARTICULATION),
    0xC9: SseqOpcodeSemantic(0xC9, "PORTAMENTO_CONTROL", SseqEffect.ARTICULATION),
    0xCA: SseqOpcodeSemantic(0xCA, "MODULATION_DEPTH", SseqEffect.MODULATION),
    0xCB: SseqOpcodeSemantic(0xCB, "MODULATION_SPEED", SseqEffect.MODULATION),
    0xCC: SseqOpcodeSemantic(0xCC, "MODULATION_TYPE", SseqEffect.MODULATION),
    0xCD: SseqOpcodeSemantic(0xCD, "MODULATION_RANGE", SseqEffect.MODULATION),
    0xCE: SseqOpcodeSemantic(0xCE, "PORTAMENTO_ON_OFF", SseqEffect.ARTICULATION),
    0xCF: SseqOpcodeSemantic(0xCF, "PORTAMENTO_TIME", SseqEffect.ARTICULATION),
    0xD0: SseqOpcodeSemantic(0xD0, "ATTACK_RATE", SseqEffect.ARTICULATION),
    0xD1: SseqOpcodeSemantic(0xD1, "DECAY_RATE", SseqEffect.ARTICULATION),
    0xD2: SseqOpcodeSemantic(0xD2, "SUSTAIN_LEVEL", SseqEffect.ARTICULATION),
    0xD3: SseqOpcodeSemantic(0xD3, "RELEASE_RATE", SseqEffect.ARTICULATION),
    0xD4: SseqOpcodeSemantic(0xD4, "LOOP_START", SseqEffect.LOOP),
    0xD5: SseqOpcodeSemantic(0xD5, "EXPRESSION", SseqEffect.DYNAMICS),
    0xD6: SseqOpcodeSemantic(0xD6, "PRINT_VARIABLE", SseqEffect.DEBUG),
    0xE0: SseqOpcodeSemantic(0xE0, "MODULATION_DELAY", SseqEffect.MODULATION),
    0xE1: SseqOpcodeSemantic(0xE1, "TEMPO", SseqEffect.TEMPO, global_scope=True),
    0xE3: SseqOpcodeSemantic(0xE3, "SWEEP_PITCH", SseqEffect.PITCH),
    0xFC: SseqOpcodeSemantic(0xFC, "LOOP_END", SseqEffect.LOOP),
    0xFD: SseqOpcodeSemantic(0xFD, "RETURN", SseqEffect.CONTROL_FLOW),
    0xFE: SseqOpcodeSemantic(0xFE, "DEFINE_TRACKS", SseqEffect.TRACK_TOPOLOGY, global_scope=True),
    0xFF: SseqOpcodeSemantic(0xFF, "END_TRACK", SseqEffect.CONTROL_FLOW, ends_track=True),
}


def classify_sseq_opcode(opcode: int) -> SseqOpcodeSemantic:
    """Classify one SSEQ opcode without decoding its variable-width payload."""

    if opcode < 0 or opcode > 0xFF:
        raise ValueError("SSEQ opcode must be in [0, 255]")
    if opcode <= 0x7F:
        return SseqOpcodeSemantic(
            opcode=opcode,
            name="NOTE",
            effect=SseqEffect.NOTE,
            encoded_pitch=opcode,
            description="note opcode encodes MIDI-like pitch; velocity and duration follow in the stream",
        )
    if opcode in _FIXED:
        return _FIXED[opcode]
    return SseqOpcodeSemantic(
        opcode=opcode,
        name=f"UNKNOWN_{opcode:02X}",
        effect=SseqEffect.UNKNOWN,
        static_risk=SseqStaticRisk.STATE_DEPENDENT,
        description="command semantics are not established by the shared DS table",
    )


def authored_path_requires_runtime_confirmation(
    semantics: tuple[SseqOpcodeSemantic, ...] | list[SseqOpcodeSemantic],
) -> bool:
    """Return true when static authored bytes cannot determine exact behavior."""

    return any(not item.static_execution_is_path_complete for item in semantics)


def musical_effects(
    semantics: tuple[SseqOpcodeSemantic, ...] | list[SseqOpcodeSemantic],
) -> frozenset[SseqEffect]:
    """Project commands onto non-spatial musical-analysis dimensions."""

    excluded = {
        SseqEffect.UNKNOWN,
        SseqEffect.CONTROL_FLOW,
        SseqEffect.VARIABLE_STATE,
        SseqEffect.RANDOMIZATION,
        SseqEffect.DEBUG,
        SseqEffect.MIX_PARAMETER,
    }
    return frozenset(item.effect for item in semantics if item.effect not in excluded)
