#!/usr/bin/env python3
"""Expose the exact-state Nuked OPN2 HQ FM lift through SourceAwareVGMPlayer.

The project-owned source-aware player already captures exact reference FM lanes.
This guarded patch adds a second six-lane sidecar generated from the same live
Nuked state. The chip still advances once. Both exact and HQ lanes use the same
outer libvgm RSMODE_LINEAR timing snapshot, including the initial upsampling
pre-generation sample, so PlayerA can replace one with the other without a
shadow engine or timeline drift.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    has_utf8_bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", has_utf8_bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, has_utf8_bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if has_utf8_bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    args = parser.parse_args()
    header = args.source_dir.resolve() / "source_aware_vgm_player.h"

    replace_once(
        header,
        """    static constexpr std::size_t kYmLaneCount = 7;
    static constexpr std::size_t kPsgLaneCount = 4;
""",
        """    static constexpr std::size_t kYmLaneCount = 7;
    static constexpr std::size_t kHqFmLaneCount = 6;
    static constexpr std::size_t kPsgLaneCount = 4;
""",
        "HQ FM lane count",
    )

    replace_once(
        header,
        """        if (m_starting) {
            promote_initial_pregen(m_ym);
            promote_initial_pregen(m_psg);
        }
""",
        """        if (m_starting) {
            promote_initial_pregen(m_ym);
            promote_initial_hq_pregen(m_ym);
            promote_initial_pregen(m_psg);
        }
""",
        "HQ FM initial upsampling pre-generation",
    )

    replace_once(
        header,
        """        m_ym_block_valid = m_render_capacity_ok
            && m_ym.expected && m_ym.attached && m_ym.timing_valid;
        m_psg_block_valid = m_render_capacity_ok
""",
        """        m_ym_block_valid = m_render_capacity_ok
            && m_ym.expected && m_ym.attached && m_ym.timing_valid;
        m_hq_fm_block_valid = m_ym_block_valid;
        m_psg_block_valid = m_render_capacity_ok
""",
        "HQ FM block start",
    )

    replace_once(
        header,
        """            if (rendered > kOutputCapacity)
                m_ym_block_valid = m_psg_block_valid = false;
""",
        """            if (rendered > kOutputCapacity) {
                m_ym_block_valid = false;
                m_hq_fm_block_valid = false;
                m_psg_block_valid = false;
            }
""",
        "HQ FM overflow invalidation",
    )

    replace_once(
        header,
        """    bool ym_source_block_valid() const noexcept { return m_ym_block_valid; }
    bool psg_source_block_valid() const noexcept { return m_psg_block_valid; }
""",
        """    bool ym_source_block_valid() const noexcept { return m_ym_block_valid; }
    bool hq_fm_source_block_valid() const noexcept { return m_hq_fm_block_valid; }
    bool psg_source_block_valid() const noexcept { return m_psg_block_valid; }
""",
        "HQ FM public validity",
    )

    replace_once(
        header,
        """    const stereo_sample* source_output(source_lane lane) const noexcept
    {
        const std::size_t index = static_cast<std::size_t>(lane);
        return index < kLaneCount ? m_output[index].data() : nullptr;
    }

protected:
""",
        """    const stereo_sample* source_output(source_lane lane) const noexcept
    {
        const std::size_t index = static_cast<std::size_t>(lane);
        return index < kLaneCount ? m_output[index].data() : nullptr;
    }

    const stereo_sample* hq_fm_source_output(std::size_t channel) const noexcept
    {
        return channel < kHqFmLaneCount ? m_hq_fm_output[channel].data() : nullptr;
    }

protected:
""",
        "HQ FM public source view",
    )

    replace_once(
        header,
        """        m_ym.nuked_state.reset();
        reset_segment_capture(m_ym);
""",
        """        m_ym.nuked_state.reset();
        reset_hq_fm_histories(m_ym);
        reset_segment_capture(m_ym);
""",
        "HQ FM reset history",
    )

    replace_once(
        header,
        """                m_ym.nuked_state.reset();
                reset_all_histories(m_ym);
                reset_segment_capture(m_ym);
""",
        """                m_ym.nuked_state.reset();
                reset_all_histories(m_ym);
                reset_hq_fm_histories(m_ym);
                reset_segment_capture(m_ym);
""",
        "HQ FM attach history",
    )

    replace_once(
        header,
        """            if (!mirror_family_segment<kYmLaneCount>(
                    m_ym, 0, outputOffset, outputCount, kYmNativeResidualTolerance))
                m_ym_block_valid = false;
""",
        """            if (!mirror_family_segment<kYmLaneCount>(
                    m_ym, 0, outputOffset, outputCount, kYmNativeResidualTolerance))
                m_ym_block_valid = false;
            if (m_hq_fm_block_valid && !mirror_hq_fm_segment(
                    m_ym, outputOffset, outputCount))
                m_hq_fm_block_valid = false;
""",
        "HQ FM outer resampler mirror",
    )

    replace_once(
        header,
        """    struct YmCapture : CaptureFamily<kYmLaneCount>
    {
        ym3438_t* chip = nullptr;
        foobar_vgm::genesis::nuked_opn2_source_resampler nuked_state{};
    };
""",
        """    struct YmCapture : CaptureFamily<kYmLaneCount>
    {
        ym3438_t* chip = nullptr;
        foobar_vgm::genesis::nuked_opn2_source_resampler nuked_state{};
        std::array<std::array<stereo_sample, kSegmentCapacity>, kHqFmLaneCount> hq_native{};
        std::array<foobar_vgm::source_audio::linear_history, kHqFmLaneCount> hq_history{};
    };
""",
        "HQ FM native capture storage",
    )

    replace_once(
        header,
        """            Bit32s mix[2] = {};
            Bit32s lanes[kYmLaneCount][2] = {};
            foobar_vgm::genesis::generate_nuked_opn2_sources(
                *capture->chip,
                capture->nuked_state,
                mix,
                lanes);
""",
        """            Bit32s mix[2] = {};
            Bit32s lanes[kYmLaneCount][2] = {};
            Bit32s hq_lanes[kHqFmLaneCount][2] = {};
            foobar_vgm::genesis::generate_nuked_opn2_sources(
                *capture->chip,
                capture->nuked_state,
                mix,
                lanes,
                hq_lanes);
""",
        "HQ FM native generation",
    )

    replace_once(
        header,
        """                for (std::size_t lane = 0; lane < kYmLaneCount; ++lane)
                    capture->native[lane][dst] = {lanes[lane][0], lanes[lane][1]};
""",
        """                for (std::size_t lane = 0; lane < kYmLaneCount; ++lane)
                    capture->native[lane][dst] = {lanes[lane][0], lanes[lane][1]};
                for (std::size_t lane = 0; lane < kHqFmLaneCount; ++lane)
                    capture->hq_native[lane][dst] = {hq_lanes[lane][0], hq_lanes[lane][1]};
""",
        "HQ FM native storage",
    )

    replace_once(
        header,
        """    template <typename Family>
    static void reset_all_histories(Family& family) noexcept
    {
        for (auto& history : family.history) history = {};
        family.mix_history = {};
    }

    template <typename Family>
""",
        """    template <typename Family>
    static void reset_all_histories(Family& family) noexcept
    {
        for (auto& history : family.history) history = {};
        family.mix_history = {};
    }

    static void reset_hq_fm_histories(YmCapture& family) noexcept
    {
        for (auto& history : family.hq_history) history = {};
    }

    static void promote_initial_hq_pregen(YmCapture& family) noexcept
    {
        if (!family.attached || !family.resampler) return;
        if (family.resampler->resampleMode != RSMODE_LINEAR
            || family.resampler->smpRateSrc >= family.resampler->smpRateDst)
            return;

        // Resmpl_Init pre-generates one native source sample for linear
        // upsampling. Exact and HQ histories must start from the same producer
        // ordinal or their first host segment would be shifted by one sample.
        if (family.overflow || family.reconstruction_error || family.native_count != 1) {
            family.timing_valid = false;
            return;
        }
        for (std::size_t lane = 0; lane < kHqFmLaneCount; ++lane) {
            family.hq_history[lane].last = {};
            family.hq_history[lane].next = family.hq_native[lane][0];
        }
    }

    bool mirror_hq_fm_segment(
        YmCapture& family,
        std::size_t outputOffset,
        std::size_t outputCount) noexcept
    {
        if (!family.timing_valid || family.overflow || family.reconstruction_error
            || family.native_count > kSegmentCapacity)
            return false;

        for (std::size_t lane = 0; lane < kHqFmLaneCount; ++lane) {
            const auto result = foobar_vgm::source_audio::mirror_linear_segment(
                family.before,
                family.hq_history[lane],
                family.hq_native[lane].data(),
                family.native_count,
                m_hq_fm_output[lane].data() + outputOffset,
                outputCount);
            if (!result.exact || result.native_consumed != family.native_count)
                return false;
        }
        return true;
    }

    template <typename Family>
""",
        "HQ FM resampler helpers",
    )

    replace_once(
        header,
        """        for (auto& lane : m_output)
            std::memset(lane.data(), 0, count * sizeof(stereo_sample));
""",
        """        for (auto& lane : m_output)
            std::memset(lane.data(), 0, count * sizeof(stereo_sample));
        for (auto& lane : m_hq_fm_output)
            std::memset(lane.data(), 0, count * sizeof(stereo_sample));
""",
        "HQ FM output clearing",
    )

    replace_once(
        header,
        """        m_output_count = 0;
        m_ym_block_valid = false;
        m_psg_block_valid = false;
""",
        """        m_output_count = 0;
        m_ym_block_valid = false;
        m_hq_fm_block_valid = false;
        m_psg_block_valid = false;
""",
        "HQ FM output invalidation",
    )

    replace_once(
        header,
        """    bool m_unsupported_genesis_topology = false;
    bool m_ym_block_valid = false;
    bool m_psg_block_valid = false;
""",
        """    bool m_unsupported_genesis_topology = false;
    bool m_ym_block_valid = false;
    bool m_hq_fm_block_valid = false;
    bool m_psg_block_valid = false;
""",
        "HQ FM validity member",
    )

    replace_once(
        header,
        """    PsgCapture m_psg{};
    std::array<std::array<stereo_sample, kOutputCapacity>, kLaneCount> m_output{};
""",
        """    PsgCapture m_psg{};
    std::array<std::array<stereo_sample, kOutputCapacity>, kLaneCount> m_output{};
    std::array<std::array<stereo_sample, kOutputCapacity>, kHqFmLaneCount> m_hq_fm_output{};
""",
        "HQ FM host output storage",
    )

    print("source-aware exact-state HQ Nuked FM lift applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
