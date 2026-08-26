#!/usr/bin/env python3
"""Add exact primary-YM2151 reference-source capture to SourceAwareVGMPlayer.

This is reference infrastructure only. It forces the primary YM2151 to the
pinned MAME core + linear resampler, attaches the guarded libvgm eight-channel
tap, mirrors those exact native lanes through the same resampler state as the
protected mix, and exposes a separate OPM source plane at the host output clock.
It does not enable or select any enhanced YM2151 renderer.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path)
    root = parser.parse_args().source_dir.resolve()
    header = root / "source_aware_vgm_player.h"

    replace_once(
        header,
        '#include "genesis_source_plane.h"\n',
        '#include "genesis_source_plane.h"\n#include "ym2151_source_plane.h"\n',
        "YM2151 source-plane include",
    )

    replace_once(
        header,
        "void sn76496_set_source_tap(void* chip, SN76496_SOURCE_TAP tap, void* user);\n}\n",
        "void sn76496_set_source_tap(void* chip, SN76496_SOURCE_TAP tap, void* user);\n"
        "typedef void (*YM2151_SOURCE_TAP)(\n"
        "    void* user,\n"
        "    const DEV_SMPL left[8],\n"
        "    const DEV_SMPL right[8],\n"
        "    DEV_SMPL mix_left,\n"
        "    DEV_SMPL mix_right);\n"
        "void ym2151_set_source_tap(void* chip, YM2151_SOURCE_TAP tap, void* user);\n}\n",
        "YM2151 source-tap ABI",
    )

    replace_once(
        header,
        "    static constexpr std::size_t kPsgLaneCount = 4;\n"
        "    static constexpr std::size_t kLaneCount = foobar_vgm::genesis::source_lane_count;\n",
        "    static constexpr std::size_t kPsgLaneCount = 4;\n"
        "    static constexpr std::size_t kOpmLaneCount = foobar_vgm::ym2151::source_lane_count;\n"
        "    static constexpr std::size_t kLaneCount = foobar_vgm::genesis::source_lane_count;\n",
        "YM2151 source count",
    )
    replace_once(
        header,
        "    static constexpr INT64 kPsgNativeResidualTolerance = 8;\n",
        "    static constexpr INT64 kPsgNativeResidualTolerance = 8;\n"
        "    static constexpr INT64 kOpmNativeResidualTolerance = 0;\n",
        "YM2151 exact residual policy",
    )

    replace_once(
        header,
        "        force_genesis_source_options();\n        clear_output_block();\n",
        "        force_genesis_source_options();\n        force_ym2151_source_options();\n        clear_output_block();\n",
        "YM2151 source option initialization",
    )

    replace_once(
        header,
        "        } else if (device_type == DEVID_SN76496) {\n"
        "            options.resmplMode = RSMODE_LINEAR;\n"
        "        }\n",
        "        } else if (device_type == DEVID_SN76496) {\n"
        "            options.resmplMode = RSMODE_LINEAR;\n"
        "        } else if (device_type == DEVID_YM2151) {\n"
        "            options.emuCore[0] = FCC_MAME;\n"
        "            options.resmplMode = RSMODE_LINEAR;\n"
        "        }\n",
        "YM2151 source device options",
    )

    replace_once(
        header,
        "        reset_segment_capture(m_ym);\n        reset_segment_capture(m_psg);\n",
        "        reset_segment_capture(m_ym);\n        reset_segment_capture(m_psg);\n"
        "        reset_segment_capture(m_opm);\n",
        "YM2151 reset capture",
    )
    replace_once(
        header,
        "        m_psg.attached = false;\n"
        "        m_ym.resampler = nullptr;\n"
        "        m_psg.resampler = nullptr;\n",
        "        m_psg.attached = false;\n"
        "        m_opm.attached = false;\n"
        "        m_ym.resampler = nullptr;\n"
        "        m_psg.resampler = nullptr;\n"
        "        m_opm.resampler = nullptr;\n",
        "YM2151 stop capture",
    )

    replace_once(
        header,
        "        m_psg_block_valid = m_render_capacity_ok\n"
        "            && m_psg.expected && m_psg.attached && m_psg.timing_valid;\n",
        "        m_psg_block_valid = m_render_capacity_ok\n"
        "            && m_psg.expected && m_psg.attached && m_psg.timing_valid;\n"
        "        m_opm_block_valid = m_render_capacity_ok\n"
        "            && m_opm.expected && m_opm.attached && m_opm.timing_valid;\n",
        "YM2151 block validity",
    )
    replace_once(
        header,
        "            if (rendered > kOutputCapacity)\n"
        "                m_ym_block_valid = m_psg_block_valid = false;\n",
        "            if (rendered > kOutputCapacity) {\n"
        "                m_ym_block_valid = m_psg_block_valid = false;\n"
        "                m_opm_block_valid = false;\n"
        "            }\n",
        "YM2151 render overflow validity",
    )

    replace_once(
        header,
        "    RuntimeDevice primary_sn76489() noexcept { return runtime_device(0x00, 0); }\n\n"
        "    bool has_nuked_ym2612() noexcept { return primary_ym2612().is_core(FCC_NUKE); }\n",
        "    RuntimeDevice primary_sn76489() noexcept { return runtime_device(0x00, 0); }\n"
        "    RuntimeDevice primary_ym2151() noexcept { return runtime_device(DEVID_YM2151, 0); }\n\n"
        "    bool has_nuked_ym2612() noexcept { return primary_ym2612().is_core(FCC_NUKE); }\n",
        "YM2151 runtime device accessor",
    )
    replace_once(
        header,
        "    bool has_default_mame_sn76489() noexcept { return primary_sn76489().is_core(FCC_MAME); }\n"
        "    bool ym_source_expected() const noexcept { return m_ym.expected; }\n",
        "    bool has_default_mame_sn76489() noexcept { return primary_sn76489().is_core(FCC_MAME); }\n"
        "    bool has_default_mame_ym2151() noexcept { return primary_ym2151().is_core(FCC_MAME); }\n"
        "    bool ym_source_expected() const noexcept { return m_ym.expected; }\n",
        "YM2151 runtime core check",
    )
    replace_once(
        header,
        "    bool psg_source_block_valid() const noexcept { return m_psg_block_valid; }\n\n"
        "    bool source_topology_supported() const noexcept\n",
        "    bool psg_source_block_valid() const noexcept { return m_psg_block_valid; }\n"
        "    bool opm_source_expected() const noexcept { return m_opm.expected; }\n"
        "    bool opm_source_block_valid() const noexcept { return m_opm_block_valid; }\n"
        "    bool opm_source_topology_supported() const noexcept\n"
        "    {\n"
        "        return m_opm.expected && !m_unsupported_opm_topology && m_opm.attached;\n"
        "    }\n\n"
        "    bool source_topology_supported() const noexcept\n",
        "YM2151 source validity API",
    )
    replace_once(
        header,
        "    const stereo_sample* source_output(source_lane lane) const noexcept\n"
        "    {\n"
        "        const std::size_t index = static_cast<std::size_t>(lane);\n"
        "        return index < kLaneCount ? m_output[index].data() : nullptr;\n"
        "    }\n",
        "    const stereo_sample* source_output(source_lane lane) const noexcept\n"
        "    {\n"
        "        const std::size_t index = static_cast<std::size_t>(lane);\n"
        "        return index < kLaneCount ? m_output[index].data() : nullptr;\n"
        "    }\n\n"
        "    const stereo_sample* opm_source_output(foobar_vgm::ym2151::source_lane lane) const noexcept\n"
        "    {\n"
        "        const std::size_t index = static_cast<std::size_t>(lane);\n"
        "        return index < kOpmLaneCount ? m_opm_output[index].data() : nullptr;\n"
        "    }\n",
        "YM2151 source output API",
    )

    replace_once(
        header,
        "            if (chipDev.chipType == DEVID_YM2612 || chipDev.chipType == DEVID_SN76496)\n"
        "                m_unsupported_genesis_topology = true;\n",
        "            if (chipDev.chipType == DEVID_YM2612 || chipDev.chipType == DEVID_SN76496)\n"
        "                m_unsupported_genesis_topology = true;\n"
        "            if (chipDev.chipType == DEVID_YM2151)\n"
        "                m_unsupported_opm_topology = true;\n",
        "YM2151 linked-device topology guard",
    )
    replace_once(
        header,
        "        if ((chipDev.chipType == DEVID_YM2612 || chipDev.chipType == DEVID_SN76496)\n"
        "            && chipDev.chipID != 0) {\n"
        "            m_unsupported_genesis_topology = true;\n"
        "            return;\n"
        "        }\n"
        "        if (chipDev.chipID != 0) return;\n",
        "        if ((chipDev.chipType == DEVID_YM2612 || chipDev.chipType == DEVID_SN76496)\n"
        "            && chipDev.chipID != 0) {\n"
        "            m_unsupported_genesis_topology = true;\n"
        "            return;\n"
        "        }\n"
        "        if (chipDev.chipType == DEVID_YM2151 && chipDev.chipID != 0) {\n"
        "            m_unsupported_opm_topology = true;\n"
        "            return;\n"
        "        }\n"
        "        if (chipDev.chipID != 0) return;\n",
        "YM2151 second-chip topology guard",
    )

    opm_attach = r'''

        if (chipDev.chipType == DEVID_YM2151) {
            m_opm.expected = true;
            if (base.defInf.devDef
                && base.defInf.devDef->coreID == FCC_MAME
                && base.resmpl.resampleMode == RSMODE_LINEAR) {
                m_opm.attached = true;
                m_opm.timing_valid = true;
                m_opm.resampler = &base.resmpl;
                reset_all_histories(m_opm);
                reset_segment_capture(m_opm);
                DEV_DATA* data = base.defInf.dataPtr;
                void* chip = data && data->chipInf ? data->chipInf : static_cast<void*>(data);
                ym2151_set_source_tap(chip, &SourceAwareVGMPlayer::opm_source_tap, &m_opm);
            }
            return;
        }
'''
    replace_once(
        header,
        "        if (chipDev.chipType == DEVID_SN76496) {\n",
        opm_attach + "\n        if (chipDev.chipType == DEVID_SN76496) {\n",
        "YM2151 source tap attachment",
    )

    replace_once(
        header,
        "        } else if (chipDev.chipType == DEVID_SN76496 && m_psg.attached\n"
        "            && m_psg.resampler == &base.resmpl) {\n"
        "            m_psg.before = base.resmpl;\n"
        "            reset_segment_capture(m_psg);\n"
        "        }\n",
        "        } else if (chipDev.chipType == DEVID_SN76496 && m_psg.attached\n"
        "            && m_psg.resampler == &base.resmpl) {\n"
        "            m_psg.before = base.resmpl;\n"
        "            reset_segment_capture(m_psg);\n"
        "        } else if (chipDev.chipType == DEVID_YM2151 && m_opm.attached\n"
        "            && m_opm.resampler == &base.resmpl) {\n"
        "            m_opm.before = base.resmpl;\n"
        "            reset_segment_capture(m_opm);\n"
        "        }\n",
        "YM2151 resample begin",
    )

    replace_once(
        header,
        "            if (chipDev.chipType == DEVID_YM2612) m_ym_block_valid = false;\n"
        "            if (chipDev.chipType == DEVID_SN76496) m_psg_block_valid = false;\n",
        "            if (chipDev.chipType == DEVID_YM2612) m_ym_block_valid = false;\n"
        "            if (chipDev.chipType == DEVID_SN76496) m_psg_block_valid = false;\n"
        "            if (chipDev.chipType == DEVID_YM2151) m_opm_block_valid = false;\n",
        "YM2151 invalid segment handling",
    )
    replace_once(
        header,
        "        } else if (chipDev.chipType == DEVID_SN76496 && m_psg.attached\n"
        "            && m_psg.resampler == &base.resmpl) {\n"
        "            if (!mirror_family_segment<kPsgLaneCount>(\n"
        "                    m_psg, kYmLaneCount, outputOffset, outputCount, kPsgNativeResidualTolerance))\n"
        "                m_psg_block_valid = false;\n"
        "        }\n",
        "        } else if (chipDev.chipType == DEVID_SN76496 && m_psg.attached\n"
        "            && m_psg.resampler == &base.resmpl) {\n"
        "            if (!mirror_family_segment<kPsgLaneCount>(\n"
        "                    m_psg, kYmLaneCount, outputOffset, outputCount, kPsgNativeResidualTolerance))\n"
        "                m_psg_block_valid = false;\n"
        "        } else if (chipDev.chipType == DEVID_YM2151 && m_opm.attached\n"
        "            && m_opm.resampler == &base.resmpl) {\n"
        "            if (!mirror_opm_segment(outputOffset, outputCount))\n"
        "                m_opm_block_valid = false;\n"
        "        }\n",
        "YM2151 resample end",
    )

    replace_once(
        header,
        "    struct PsgCapture : CaptureFamily<kPsgLaneCount> {};\n",
        "    struct PsgCapture : CaptureFamily<kPsgLaneCount> {};\n"
        "    struct OpmCapture : CaptureFamily<kOpmLaneCount> {};\n",
        "YM2151 capture family",
    )

    opm_callback = r'''

    static void opm_source_tap(
        void* opaque,
        const DEV_SMPL left[8],
        const DEV_SMPL right[8],
        DEV_SMPL mix_left,
        DEV_SMPL mix_right)
    {
        OpmCapture* capture = static_cast<OpmCapture*>(opaque);
        if (!capture || !left || !right) return;

        INT64 sum_l = 0;
        INT64 sum_r = 0;
        for (std::size_t lane = 0; lane < kOpmLaneCount; ++lane) {
            sum_l += left[lane];
            sum_r += right[lane];
        }
        if (sum_l != static_cast<INT64>(mix_left)
            || sum_r != static_cast<INT64>(mix_right))
            capture->reconstruction_error = true;

        const std::size_t dst = capture->native_count;
        if (dst < kSegmentCapacity) {
            capture->native_mix[dst] = {mix_left, mix_right};
            for (std::size_t lane = 0; lane < kOpmLaneCount; ++lane)
                capture->native[lane][dst] = {left[lane], right[lane]};
        } else {
            capture->overflow = true;
        }
        ++capture->native_count;
    }
'''
    replace_once(
        header,
        "    template <typename Family>\n    static void reset_segment_capture(Family& family) noexcept\n",
        opm_callback + "\n    template <typename Family>\n    static void reset_segment_capture(Family& family) noexcept\n",
        "YM2151 native callback",
    )

    opm_mirror = r'''

    bool mirror_opm_segment(
        std::size_t outputOffset,
        std::size_t outputCount) noexcept
    {
        auto& family = m_opm;
        if (!family.timing_valid || family.overflow || family.reconstruction_error
            || family.native_count > kSegmentCapacity) {
            family.timing_valid = false;
            return false;
        }

        const auto mix_result = foobar_vgm::source_audio::mirror_linear_segment(
            family.before,
            family.mix_history,
            family.native_mix.data(),
            family.native_count,
            family.expected_output.data(),
            outputCount);
        if (!mix_result.exact || mix_result.native_consumed != family.native_count) {
            family.timing_valid = false;
            return false;
        }

        for (std::size_t lane = 0; lane < kOpmLaneCount; ++lane) {
            const auto result = foobar_vgm::source_audio::mirror_linear_segment(
                family.before,
                family.history[lane],
                family.native[lane].data(),
                family.native_count,
                m_opm_output[lane].data() + outputOffset,
                outputCount);
            if (!result.exact || result.native_consumed != family.native_count) {
                family.timing_valid = false;
                return false;
            }
        }

        const INT64 tolerance_l = output_residual_tolerance(
            kOpmNativeResidualTolerance, family.before.volumeL, kOpmLaneCount);
        const INT64 tolerance_r = output_residual_tolerance(
            kOpmNativeResidualTolerance, family.before.volumeR, kOpmLaneCount);
        for (std::size_t frame = 0; frame < outputCount; ++frame) {
            INT64 sum_l = 0;
            INT64 sum_r = 0;
            for (std::size_t lane = 0; lane < kOpmLaneCount; ++lane) {
                const auto& sample = m_opm_output[lane][outputOffset + frame];
                sum_l += sample.left;
                sum_r += sample.right;
            }
            const auto& expected = family.expected_output[frame];
            if (abs64(static_cast<INT64>(expected.left) - sum_l) > tolerance_l
                || abs64(static_cast<INT64>(expected.right) - sum_r) > tolerance_r) {
                family.timing_valid = false;
                return false;
            }
        }
        return true;
    }
'''
    replace_once(
        header,
        "    void force_genesis_source_options()\n",
        opm_mirror + "\n    void force_genesis_source_options()\n",
        "YM2151 host-rate source mirroring",
    )

    force_opm = r'''

    void force_ym2151_source_options()
    {
        for (UINT8 instance = 0; instance < 2; ++instance) {
            const UINT32 opm_id = PLR_DEV_ID(DEVID_YM2151, instance);
            PLR_DEV_OPTS opm{};
            if (VGMPlayer::GetDeviceOptions(opm_id, opm) == 0x00) {
                opm.emuCore[0] = FCC_MAME;
                opm.resmplMode = RSMODE_LINEAR;
                VGMPlayer::SetDeviceOptions(opm_id, opm);
            }
        }
    }
'''
    replace_once(
        header,
        "    void reset_for_new_device_generation() noexcept\n",
        force_opm + "\n    void reset_for_new_device_generation() noexcept\n",
        "YM2151 forced reference options",
    )
    replace_once(
        header,
        "        m_psg = {};\n        m_unsupported_genesis_topology = false;\n",
        "        m_psg = {};\n"
        "        m_opm = {};\n"
        "        m_unsupported_genesis_topology = false;\n"
        "        m_unsupported_opm_topology = false;\n",
        "YM2151 generation reset",
    )
    replace_once(
        header,
        "        for (auto& lane : m_output)\n"
        "            std::memset(lane.data(), 0, count * sizeof(stereo_sample));\n",
        "        for (auto& lane : m_output)\n"
        "            std::memset(lane.data(), 0, count * sizeof(stereo_sample));\n"
        "        for (auto& lane : m_opm_output)\n"
        "            std::memset(lane.data(), 0, count * sizeof(stereo_sample));\n",
        "YM2151 output clear",
    )
    replace_once(
        header,
        "    void invalidate_output_block() noexcept\n"
        "    {\n"
        "        m_output_count = 0;\n"
        "        m_ym_block_valid = false;\n"
        "        m_psg_block_valid = false;\n"
        "    }\n",
        "    void invalidate_output_block() noexcept\n"
        "    {\n"
        "        m_output_count = 0;\n"
        "        m_ym_block_valid = false;\n"
        "        m_psg_block_valid = false;\n"
        "        m_opm_block_valid = false;\n"
        "    }\n",
        "YM2151 output invalidation",
    )
    replace_once(
        header,
        "    bool m_unsupported_genesis_topology = false;\n"
        "    bool m_ym_block_valid = false;\n"
        "    bool m_psg_block_valid = false;\n",
        "    bool m_unsupported_genesis_topology = false;\n"
        "    bool m_unsupported_opm_topology = false;\n"
        "    bool m_ym_block_valid = false;\n"
        "    bool m_psg_block_valid = false;\n"
        "    bool m_opm_block_valid = false;\n",
        "YM2151 persistent validity state",
    )
    replace_once(
        header,
        "    YmCapture m_ym{};\n"
        "    PsgCapture m_psg{};\n"
        "    std::array<std::array<stereo_sample, kOutputCapacity>, kLaneCount> m_output{};\n",
        "    YmCapture m_ym{};\n"
        "    PsgCapture m_psg{};\n"
        "    OpmCapture m_opm{};\n"
        "    std::array<std::array<stereo_sample, kOutputCapacity>, kLaneCount> m_output{};\n"
        "    std::array<std::array<stereo_sample, kOutputCapacity>, kOpmLaneCount> m_opm_output{};\n",
        "YM2151 capture storage",
    )

    print("foo_input_vgm exact YM2151 reference capture applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
