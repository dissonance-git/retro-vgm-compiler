#!/usr/bin/env python3
"""Attach the non-audible Studio FIR readiness observer to the HQ Nuked FM lift.

This patch deliberately does not change PlayerA output. It turns the live six-lane
HQ native producer into exact ordinal-tagged Studio FM frames plus explicit
startup/tail reference evidence. Audible promotion remains a separate transport
step so whole protected source-family bundles can be delayed together.
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
        '"nuked_opn2_source_capture.h"\n',
        '"nuked_opn2_source_capture.h"\n#include "studio_hq_fm_observer.h"\n',
        "Studio HQ FM observer include",
    )

    replace_once(
        header,
        """    UINT8 Reset() override
    {
        if (m_starting) {
            promote_initial_pregen(m_ym);
            promote_initial_hq_pregen(m_ym);
            promote_initial_pregen(m_psg);
        }
""",
        """    UINT8 Reset() override
    {
        if (!m_starting) {
            m_studio_hq_fm_observer.reset();
            m_studio_hq_fm_active = false;
            m_studio_hq_fm_last = {};
        }
        if (m_starting) {
            promote_initial_pregen(m_ym);
            promote_initial_hq_pregen(m_ym);
            observe_initial_studio_hq_pregen(m_ym);
            promote_initial_pregen(m_psg);
        }
""",
        "Studio HQ FM reset lifecycle",
    )

    replace_once(
        header,
        """    UINT8 Seek(UINT8 unit, UINT32 pos) override
    {
        const UINT8 result = VGMPlayer::Seek(unit, pos);
        invalidate_output_block();
        return result;
    }
""",
        """    UINT8 Seek(UINT8 unit, UINT32 pos) override
    {
        const UINT8 result = VGMPlayer::Seek(unit, pos);
        m_studio_hq_fm_observer.reset();
        m_studio_hq_fm_active = false;
        m_studio_hq_fm_last = {};
        invalidate_output_block();
        return result;
    }
""",
        "Studio HQ FM seek fail closed",
    )

    replace_once(
        header,
        """    bool ym_source_block_valid() const noexcept { return m_ym_block_valid; }
    bool hq_fm_source_block_valid() const noexcept { return m_hq_fm_block_valid; }
    bool psg_source_block_valid() const noexcept { return m_psg_block_valid; }
""",
        """    bool ym_source_block_valid() const noexcept { return m_ym_block_valid; }
    bool hq_fm_source_block_valid() const noexcept { return m_hq_fm_block_valid; }
    using studio_hq_fm_ready_frame = foobar_vgm::source_audio::studio_hq_fm_ready_frame<
        kHqFmLaneCount>;
    bool studio_hq_fm_observer_valid() const noexcept
    {
        return m_studio_hq_fm_active && m_studio_hq_fm_observer.valid();
    }
    bool studio_hq_fm_domain_started() const noexcept
    {
        return studio_hq_fm_observer_valid()
            && m_studio_hq_fm_observer.studio_domain_started();
    }
    std::uint64_t studio_hq_fm_first_destination_ordinal() const noexcept
    {
        return studio_hq_fm_domain_started()
            ? m_studio_hq_fm_observer.first_studio_destination_ordinal()
            : 0;
    }
    std::uint64_t studio_hq_fm_next_destination_ordinal() const noexcept
    {
        return studio_hq_fm_observer_valid()
            ? m_studio_hq_fm_observer.next_destination_ordinal()
            : 0;
    }
    std::uint64_t studio_hq_fm_next_release_ordinal() const noexcept
    {
        return studio_hq_fm_observer_valid()
            ? m_studio_hq_fm_observer.next_release_ordinal()
            : 0;
    }
    const foobar_vgm::source_audio::studio_hq_fm_observation&
    studio_hq_fm_last_observation() const noexcept
    {
        return m_studio_hq_fm_last;
    }
    std::size_t studio_hq_fm_ready_frames() const noexcept
    {
        return studio_hq_fm_observer_valid() ? m_studio_hq_fm_observer.ready_frames() : 0;
    }
    bool pop_studio_hq_fm_ready_frame(studio_hq_fm_ready_frame& out) noexcept
    {
        return studio_hq_fm_observer_valid()
            && m_studio_hq_fm_observer.pop_ready_frame(out);
    }
    std::size_t finish_studio_hq_fm_reference_tail() noexcept
    {
        return studio_hq_fm_observer_valid()
            ? m_studio_hq_fm_observer.finish_reference_tail()
            : 0;
    }
    bool psg_source_block_valid() const noexcept { return m_psg_block_valid; }
""",
        "Studio HQ FM diagnostic view",
    )

    replace_once(
        header,
        """                m_ym.nuked_state.reset();
                reset_all_histories(m_ym);
                reset_hq_fm_histories(m_ym);
                reset_segment_capture(m_ym);
                base.resmpl.StreamUpdate = &SourceAwareVGMPlayer::ym_stream_update;
""",
        """                m_ym.nuked_state.reset();
                reset_all_histories(m_ym);
                reset_hq_fm_histories(m_ym);
                reset_segment_capture(m_ym);
                m_studio_hq_fm_last = {};
                m_studio_hq_fm_active = m_studio_hq_fm_observer.configure(
                    base.resmpl.smpRateSrc,
                    base.resmpl.smpRateDst);
                base.resmpl.StreamUpdate = &SourceAwareVGMPlayer::ym_stream_update;
""",
        "Studio HQ FM device attachment",
    )

    replace_once(
        header,
        """            if (m_hq_fm_block_valid && !mirror_hq_fm_segment(
                    m_ym, outputOffset, outputCount))
                m_hq_fm_block_valid = false;
""",
        """            if (m_hq_fm_block_valid && !mirror_hq_fm_segment(
                    m_ym, outputOffset, outputCount))
                m_hq_fm_block_valid = false;
            if (m_studio_hq_fm_active && !observe_studio_hq_fm_segment(
                    m_ym, outputCount))
                m_studio_hq_fm_active = false;
""",
        "Studio HQ FM live observation",
    )

    replace_once(
        header,
        """    bool mirror_hq_fm_segment(
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
        """    bool mirror_hq_fm_segment(
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

    void observe_initial_studio_hq_pregen(YmCapture& family) noexcept
    {
        if (!m_studio_hq_fm_active || !family.resampler)
            return;
        if (family.resampler->resampleMode != RSMODE_LINEAR
            || family.resampler->smpRateSrc >= family.resampler->smpRateDst)
            return;
        if (family.overflow || family.reconstruction_error || family.native_count != 1) {
            m_studio_hq_fm_active = false;
            return;
        }

        std::array<const stereo_sample*, kHqFmLaneCount> lanes{};
        for (std::size_t lane = 0; lane < kHqFmLaneCount; ++lane)
            lanes[lane] = family.hq_native[lane].data();
        if (!m_studio_hq_fm_observer.append_initial_pregeneration(lanes, 1))
            m_studio_hq_fm_active = false;
    }

    bool observe_studio_hq_fm_segment(
        YmCapture& family,
        std::size_t outputCount) noexcept
    {
        if (!family.timing_valid || family.overflow || family.reconstruction_error
            || family.native_count > kSegmentCapacity)
            return false;

        foobar_vgm::source_audio::studio_linear_timing_snapshot timing{};
        timing.source_rate_hz = family.before.smpRateSrc;
        timing.destination_rate_hz = family.before.smpRateDst;
        timing.sample_p = family.before.smpP;
        timing.sample_last = family.before.smpLast;
        timing.sample_next = family.before.smpNext;

        std::array<const stereo_sample*, kHqFmLaneCount> lanes{};
        for (std::size_t lane = 0; lane < kHqFmLaneCount; ++lane)
            lanes[lane] = family.hq_native[lane].data();

        const foobar_vgm::source_audio::studio_hq_fm_gain gain{
            static_cast<std::int32_t>(family.before.volumeL),
            static_cast<std::int32_t>(family.before.volumeR)
        };
        m_studio_hq_fm_last = m_studio_hq_fm_observer.observe_segment(
            timing,
            lanes,
            family.native_count,
            outputCount,
            gain);
        return m_studio_hq_fm_last.valid;
    }

    template <typename Family>
""",
        "Studio HQ FM observer helpers",
    )

    replace_once(
        header,
        """    void reset_for_new_device_generation() noexcept
    {
        m_ym = {};
        m_psg = {};
        m_unsupported_genesis_topology = false;
""",
        """    void reset_for_new_device_generation() noexcept
    {
        m_studio_hq_fm_observer.reset();
        m_studio_hq_fm_active = false;
        m_studio_hq_fm_last = {};
        m_ym = {};
        m_psg = {};
        m_unsupported_genesis_topology = false;
""",
        "Studio HQ FM generation reset",
    )

    replace_once(
        header,
        """    std::size_t m_output_count = 0;

    YmCapture m_ym{};
""",
        """    std::size_t m_output_count = 0;
    using studio_hq_fm_observer_type = foobar_vgm::source_audio::studio_hq_fm_observer<
        kHqFmLaneCount,
        kSegmentCapacity + foobar_vgm::source_audio::studio_source_resampler_kernel::tap_count * 2,
        kOutputCapacity + foobar_vgm::source_audio::studio_source_resampler_kernel::tap_count>;
    studio_hq_fm_observer_type m_studio_hq_fm_observer{};
    foobar_vgm::source_audio::studio_hq_fm_observation m_studio_hq_fm_last{};
    bool m_studio_hq_fm_active = false;

    YmCapture m_ym{};
""",
        "Studio HQ FM observer state",
    )

    print("non-audible Studio HQ FM observer applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
