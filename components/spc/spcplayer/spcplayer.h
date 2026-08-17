/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "../snesapu_source_wire_v2.h"

#include <cstdint>

#define SPCP_HEADER_SIGNATURE "SPCP"
#define SPCP_HEADER_VERSION    1
#define SPCP_HEADER_SIZE       32

// Historical process envelope:
#if 0
// Header begin
uint32_t signature;
uint32_t version;
uint32_t size;
uint32_t spc_size;
uint32_t script700_size;
uint32_t reserved1;
uint32_t reserved2;
uint32_t reserved3;
// Header end

uint8_t[] spc_data;
uint8_t[] script700_data;
#endif

// ---------------------------------------------------------------------------
// Source/presentation telemetry extension
// Written after each reference PCM block when --telemetry is active.
// ---------------------------------------------------------------------------
#define SPCP_TELEM_MAGIC    0x4D454C54u  // "TLEM" as uint32 LE
#define SPCP_TELEM_VOICES   8

// SRCE v2 is owned by one repository-level process ABI. Keep these compatibility
// names because the historical foo_snesapu controller includes spcplayer.h, but
// do not duplicate the wire values or layout here.
using SpcSourceWire = gameaudio::spc::snesapu_source_wire_v2;
using SpcSourceBlockHeader = SpcSourceWire::header;

inline constexpr std::uint32_t SPCP_SOURCE_MAGIC = SpcSourceWire::magic;
inline constexpr std::uint16_t SPCP_SOURCE_VERSION = SpcSourceWire::version;
inline constexpr std::size_t SPCP_SOURCE_VOICES = SpcSourceWire::voice_count;
inline constexpr std::size_t SPCP_SOURCE_DRY_BASE = SpcSourceWire::dry_base;
inline constexpr std::size_t SPCP_SOURCE_GAIN_L_BASE = SpcSourceWire::gain_left_base;
inline constexpr std::size_t SPCP_SOURCE_GAIN_R_BASE = SpcSourceWire::gain_right_base;
inline constexpr std::size_t SPCP_SOURCE_ECHO_L = SpcSourceWire::echo_left_plane;
inline constexpr std::size_t SPCP_SOURCE_ECHO_R = SpcSourceWire::echo_right_plane;
inline constexpr std::size_t SPCP_SOURCE_AUDIO_LANES = SpcSourceWire::audio_lane_count;
inline constexpr std::size_t SPCP_SOURCE_PLANES = SpcSourceWire::plane_count;
inline constexpr std::size_t SPCP_SOURCE_MAX_FRAMES = SpcSourceWire::max_frames;
inline constexpr std::uint16_t SPCP_SOURCE_FORMAT_F32 = SpcSourceWire::format_float32;

constexpr std::uint32_t spcp_stream_block_samples(
    std::uint32_t sample_rate,
    bool sources) noexcept
{
    return SpcSourceWire::stream_block_samples(sample_rate, sources);
}

#pragma pack(push, 1)

// Per-voice block-end telemetry is historical S-DSP evidence. It is not the
// causal source plane and must not substitute for the sample-exact SRCE control
// trajectories.
struct SpcVoiceTelem
{
    std::uint8_t  mFlg;
    std::uint8_t  eMode;
    std::uint8_t  is_echo;
    std::uint8_t  is_noise;
    std::int8_t   volL;
    std::int8_t   volR;
    std::int8_t   envx;
    std::int8_t   outx;
    std::uint16_t pitch;
    std::uint8_t  srcn;
    std::uint8_t  _pad;
    std::uint32_t eVal;
};
static_assert(sizeof(SpcVoiceTelem) == 16, "SpcVoiceTelem must be 16 bytes");

struct SpcEchoTelem
{
    std::int8_t   evolL;
    std::int8_t   evolR;
    std::int8_t   efb;
    std::uint8_t  edl;
    std::uint8_t  eon_mask;
    std::uint8_t  flg;
    std::uint8_t  _pad[2];
};
static_assert(sizeof(SpcEchoTelem) == 8, "SpcEchoTelem must be 8 bytes");

struct SpcBlockTelem
{
    std::uint32_t magic;
    std::uint32_t block_samples;
    SpcEchoTelem  echo;
    std::uint8_t  _pad[4];
    SpcVoiceTelem voice[SPCP_TELEM_VOICES];
};
static_assert(
    sizeof(SpcBlockTelem) == 8 + 8 + 4 + 16 * SPCP_TELEM_VOICES,
    "SpcBlockTelem size mismatch");

#pragma pack(pop)

static_assert(
    sizeof(SpcSourceBlockHeader) == 24,
    "shared SNESAPU SRCE v2 header ABI changed");
