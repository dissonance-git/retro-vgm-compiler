#pragma once

#include <player/vgmplayer.hpp>
#include <emu/EmuCores.h>
#include <emu/SoundDevs.h>
#include "genesis_source_plane.h"
#include "linear_source_resampler.h"
#include "nuked_opn2_source_capture.h"

#include <array>
#include <cstddef>
#include <cstring>

// Supplied by patches/libvgm/apply_sn76496_source_tap.py before libvgm builds.
extern "C" {
typedef void (*SN76496_SOURCE_TAP)(
    void* user,
    const DEV_SMPL left[4],
    const DEV_SMPL right[4],
    DEV_SMPL mix_left,
    DEV_SMPL mix_right);
void sn76496_set_source_tap(void* chip, SN76496_SOURCE_TAP tap, void* user);
}

// Source-aware VGMPlayer for the private foobar component.
//
// The protected stereo mix still comes from the normal VGMPlayer/Resampler
// path. The private VGMPlayer hooks added by patches/libvgm let this derived
// class observe the exact same native samples and exact same output-rate segment
// boundaries without a shadow emulator or advancing a chip twice.
class SourceAwareVGMPlayer final : public VGMPlayer
{
public:
    using stereo_sample = foobar_vgm::source_audio::stereo_sample;
    using source_lane = foobar_vgm::genesis::source_lane;

    static constexpr std::size_t kYmLaneCount = 7;
    static constexpr std::size_t kPsgLaneCount = 4;
    static constexpr std::size_t kLaneCount = foobar_vgm::genesis::source_lane_count;
    static constexpr std::size_t kSegmentCapacity = 8192;
    static constexpr std::size_t kOutputCapacity = 8192;
    static constexpr INT64 kYmNativeResidualTolerance = 32;
    static constexpr INT64 kPsgNativeResidualTolerance = 8;

    struct RuntimeDevice
    {
        void* chip = nullptr;
        UINT32 core_id = 0;
        UINT32 sample_rate = 0;
        DEV_ID device_type = 0;
        UINT8 instance = 0;

        explicit operator bool() const noexcept { return chip != nullptr; }
        bool is_core(UINT32 expected) const noexcept {
            return chip != nullptr && core_id == expected;
        }
    };

    SourceAwareVGMPlayer()
    {
        force_genesis_source_options();
        clear_output_block();
    }

    UINT8 SetDeviceOptions(UINT32 id, const PLR_DEV_OPTS& requested) override
    {
        PLR_DEV_OPTS options = requested;
        const UINT32 device_type = id & 0xFFu;
        if (device_type == DEVID_YM2612) {
            options.emuCore[0] = FCC_NUKE;
            options.resmplMode = RSMODE_LINEAR;
        } else if (device_type == DEVID_SN76496) {
            options.resmplMode = RSMODE_LINEAR;
        }
        return VGMPlayer::SetDeviceOptions(id, options);
    }

    UINT8 Start() override
    {
        reset_for_new_device_generation();
        m_starting = true;
        const UINT8 result = VGMPlayer::Start();
        m_starting = false;
        return result;
    }

    UINT8 Reset() override
    {
        if (m_starting) {
            promote_initial_pregen(m_ym);
            promote_initial_pregen(m_psg);
        }

        const UINT8 result = VGMPlayer::Reset();
        m_ym.nuked_state.reset();
        reset_segment_capture(m_ym);
        reset_segment_capture(m_psg);
        invalidate_output_block();
        return result;
    }

    UINT8 Seek(UINT8 unit, UINT32 pos) override
    {
        const UINT8 result = VGMPlayer::Seek(unit, pos);
        invalidate_output_block();
        return result;
    }

    UINT8 Stop() override
    {
        const UINT8 result = VGMPlayer::Stop();
        m_ym.attached = false;
        m_psg.attached = false;
        m_ym.resampler = nullptr;
        m_psg.resampler = nullptr;
        invalidate_output_block();
        return result;
    }

    UINT32 Render(UINT32 smplCnt, WAVE_32BS* data) override
    {
        m_render_capacity_ok = smplCnt <= kOutputCapacity;
        m_output_count = m_render_capacity_ok ? smplCnt : 0;
        m_ym_block_valid = m_render_capacity_ok
            && m_ym.expected && m_ym.attached && m_ym.timing_valid;
        m_psg_block_valid = m_render_capacity_ok
            && m_psg.expected && m_psg.attached && m_psg.timing_valid;
        if (m_render_capacity_ok)
            clear_output_block(smplCnt);

        const UINT32 rendered = VGMPlayer::Render(smplCnt, data);
        if (m_render_capacity_ok) {
            m_output_count = rendered <= kOutputCapacity ? rendered : 0;
            if (rendered > kOutputCapacity)
                m_ym_block_valid = m_psg_block_valid = false;
        }
        return rendered;
    }

    RuntimeDevice runtime_device(UINT8 vgm_chip_type, UINT8 chip_id = 0) noexcept
    {
        CHIP_DEVICE* dev = GetDevicePtr(vgm_chip_type, chip_id);
        if (!dev || !dev->base.defInf.dataPtr || !dev->base.defInf.devDef)
            return {};

        RuntimeDevice out;
        DEV_DATA* data = dev->base.defInf.dataPtr;
        out.chip = data->chipInf ? data->chipInf : static_cast<void*>(data);
        out.core_id = dev->base.defInf.devDef->coreID;
        out.sample_rate = dev->base.defInf.sampleRate;
        out.device_type = dev->chipType;
        out.instance = dev->chipID;
        return out;
    }

    RuntimeDevice primary_ym2612() noexcept { return runtime_device(0x02, 0); }
    RuntimeDevice primary_sn76489() noexcept { return runtime_device(0x00, 0); }

    bool has_nuked_ym2612() noexcept { return primary_ym2612().is_core(FCC_NUKE); }
    bool has_default_mame_sn76489() noexcept { return primary_sn76489().is_core(FCC_MAME); }
    bool ym_source_expected() const noexcept { return m_ym.expected; }
    bool psg_source_expected() const noexcept { return m_psg.expected; }
    bool ym_source_block_valid() const noexcept { return m_ym_block_valid; }
    bool psg_source_block_valid() const noexcept { return m_psg_block_valid; }

    bool source_topology_supported() const noexcept
    {
        // Source admission is intentionally chip-scoped. It depends on the VGM
        // device type/core/instance wiring below. No VGM system/platform metadata is consulted.
        const bool any = m_ym.expected || m_psg.expected;
        return any && !m_unsupported_genesis_topology
            && (!m_ym.expected || m_ym.attached)
            && (!m_psg.expected || m_psg.attached);
    }

    bool source_block_complete() const noexcept
    {
        return m_render_capacity_ok && source_topology_supported()
            && (!m_ym.expected || m_ym_block_valid)
            && (!m_psg.expected || m_psg_block_valid);
    }

    std::size_t source_output_count() const noexcept { return m_output_count; }

    const stereo_sample* source_output(source_lane lane) const noexcept
    {
        const std::size_t index = static_cast<std::size_t>(lane);
        return index < kLaneCount ? m_output[index].data() : nullptr;
    }

protected:
    void SourceTapOnResamplerConnected(
        CHIP_DEVICE& chipDev,
        VGM_BASEDEV& base,
        UINT8 linkIndex) override
    {
        if (linkIndex != 0) {
            if (chipDev.chipType == DEVID_YM2612 || chipDev.chipType == DEVID_SN76496)
                m_unsupported_genesis_topology = true;
            return;
        }

        if ((chipDev.chipType == DEVID_YM2612 || chipDev.chipType == DEVID_SN76496)
            && chipDev.chipID != 0) {
            m_unsupported_genesis_topology = true;
            return;
        }
        if (chipDev.chipID != 0) return;

        if (chipDev.chipType == DEVID_YM2612) {
            m_ym.expected = true;
            if (base.defInf.devDef
                && base.defInf.devDef->coreID == FCC_NUKE
                && base.resmpl.resampleMode == RSMODE_LINEAR) {
                m_ym.attached = true;
                m_ym.timing_valid = true;
                m_ym.resampler = &base.resmpl;
                DEV_DATA* data = base.defInf.dataPtr;
                void* chip = data && data->chipInf ? data->chipInf : static_cast<void*>(data);
                m_ym.chip = reinterpret_cast<ym3438_t*>(chip);
                m_ym.nuked_state.reset();
                reset_all_histories(m_ym);
                reset_segment_capture(m_ym);
                base.resmpl.StreamUpdate = &SourceAwareVGMPlayer::ym_stream_update;
                base.resmpl.su_DataPtr = &m_ym;
            }
            return;
        }

        if (chipDev.chipType == DEVID_SN76496) {
            m_psg.expected = true;
            if (base.defInf.devDef
                && base.defInf.devDef->coreID == FCC_MAME
                && base.resmpl.resampleMode == RSMODE_LINEAR) {
                m_psg.attached = true;
                m_psg.timing_valid = true;
                m_psg.resampler = &base.resmpl;
                reset_all_histories(m_psg);
                reset_segment_capture(m_psg);
                sn76496_set_source_tap(
                    base.defInf.dataPtr,
                    &SourceAwareVGMPlayer::psg_source_tap,
                    &m_psg);
            }
        }
    }

    void SourceTapOnResampleBegin(
        CHIP_DEVICE& chipDev,
        VGM_BASEDEV& base,
        UINT32,
        UINT32 outputCount) override
    {
        if (chipDev.chipID != 0 || outputCount == 0) return;
        if (chipDev.chipType == DEVID_YM2612 && m_ym.attached
            && m_ym.resampler == &base.resmpl) {
            m_ym.before = base.resmpl;
            reset_segment_capture(m_ym);
        } else if (chipDev.chipType == DEVID_SN76496 && m_psg.attached
            && m_psg.resampler == &base.resmpl) {
            m_psg.before = base.resmpl;
            reset_segment_capture(m_psg);
        }
    }

    void SourceTapOnResampleEnd(
        CHIP_DEVICE& chipDev,
        VGM_BASEDEV& base,
        UINT32 outputOffset,
        UINT32 outputCount) override
    {
        if (outputCount == 0) return;

        if (chipDev.chipID != 0 || !m_render_capacity_ok
            || outputOffset > kOutputCapacity
            || outputCount > kOutputCapacity - outputOffset) {
            if (chipDev.chipType == DEVID_YM2612) m_ym_block_valid = false;
            if (chipDev.chipType == DEVID_SN76496) m_psg_block_valid = false;
            return;
        }

        if (chipDev.chipType == DEVID_YM2612 && m_ym.attached
            && m_ym.resampler == &base.resmpl) {
            if (!mirror_family_segment<kYmLaneCount>(
                    m_ym, 0, outputOffset, outputCount, kYmNativeResidualTolerance))
                m_ym_block_valid = false;
        } else if (chipDev.chipType == DEVID_SN76496 && m_psg.attached
            && m_psg.resampler == &base.resmpl) {
            if (!mirror_family_segment<kPsgLaneCount>(
                    m_psg, kYmLaneCount, outputOffset, outputCount, kPsgNativeResidualTolerance))
                m_psg_block_valid = false;
        }
    }

private:
    template <std::size_t LaneCount>
    struct CaptureFamily
    {
        bool expected = false;
        bool attached = false;
        bool timing_valid = true;
        bool overflow = false;
        bool reconstruction_error = false;
        std::size_t native_count = 0;
        RESMPL_STATE* resampler = nullptr;
        RESMPL_STATE before{};
        std::array<std::array<stereo_sample, kSegmentCapacity>, LaneCount> native{};
        std::array<stereo_sample, kSegmentCapacity> native_mix{};
        std::array<foobar_vgm::source_audio::linear_history, LaneCount> history{};
        foobar_vgm::source_audio::linear_history mix_history{};
        std::array<stereo_sample, kOutputCapacity> expected_output{};
    };

    struct YmCapture : CaptureFamily<kYmLaneCount>
    {
        ym3438_t* chip = nullptr;
        foobar_vgm::genesis::nuked_opn2_source_resampler nuked_state{};
    };

    struct PsgCapture : CaptureFamily<kPsgLaneCount> {};

    static INT64 abs64(INT64 value) noexcept { return value < 0 ? -value : value; }

    static INT64 output_residual_tolerance(
        INT64 native_tolerance,
        INT16 volume,
        std::size_t lane_count) noexcept
    {
        return native_tolerance * abs64(static_cast<INT64>(volume))
            + static_cast<INT64>(lane_count + 2);
    }

    static void ym_stream_update(void* opaque, UINT32 samples, DEV_SMPL** outputs)
    {
        YmCapture* capture = static_cast<YmCapture*>(opaque);
        if (!capture || !capture->chip || !outputs || !outputs[0] || !outputs[1])
            return;

        for (UINT32 sample = 0; sample < samples; ++sample) {
            Bit32s mix[2] = {};
            Bit32s lanes[kYmLaneCount][2] = {};
            foobar_vgm::genesis::generate_nuked_opn2_sources(
                *capture->chip,
                capture->nuked_state,
                mix,
                lanes);
            outputs[0][sample] = mix[0];
            outputs[1][sample] = mix[1];

            const Bit32s residual_l = foobar_vgm::genesis::source_accounting_residual(
                mix[0], lanes, 0);
            const Bit32s residual_r = foobar_vgm::genesis::source_accounting_residual(
                mix[1], lanes, 1);
            if (abs64(static_cast<INT64>(residual_l)) > kYmNativeResidualTolerance
                || abs64(static_cast<INT64>(residual_r)) > kYmNativeResidualTolerance)
                capture->reconstruction_error = true;

            const std::size_t dst = capture->native_count;
            if (dst < kSegmentCapacity) {
                capture->native_mix[dst] = {mix[0], mix[1]};
                for (std::size_t lane = 0; lane < kYmLaneCount; ++lane)
                    capture->native[lane][dst] = {lanes[lane][0], lanes[lane][1]};
            } else {
                capture->overflow = true;
            }
            ++capture->native_count;
        }
    }

    static void psg_source_tap(
        void* opaque,
        const DEV_SMPL left[4],
        const DEV_SMPL right[4],
        DEV_SMPL mix_left,
        DEV_SMPL mix_right)
    {
        PsgCapture* capture = static_cast<PsgCapture*>(opaque);
        if (!capture || !left || !right) return;

        INT64 sum_l = 0;
        INT64 sum_r = 0;
        for (std::size_t lane = 0; lane < kPsgLaneCount; ++lane) {
            sum_l += left[lane];
            sum_r += right[lane];
        }
        if (abs64(static_cast<INT64>(mix_left) - sum_l) > kPsgNativeResidualTolerance
            || abs64(static_cast<INT64>(mix_right) - sum_r) > kPsgNativeResidualTolerance)
            capture->reconstruction_error = true;

        const std::size_t dst = capture->native_count;
        if (dst < kSegmentCapacity) {
            capture->native_mix[dst] = {mix_left, mix_right};
            for (std::size_t lane = 0; lane < kPsgLaneCount; ++lane)
                capture->native[lane][dst] = {left[lane], right[lane]};
        } else {
            capture->overflow = true;
        }
        ++capture->native_count;
    }

    template <typename Family>
    static void reset_segment_capture(Family& family) noexcept
    {
        family.native_count = 0;
        family.overflow = false;
        family.reconstruction_error = false;
    }

    template <typename Family>
    static void reset_all_histories(Family& family) noexcept
    {
        for (auto& history : family.history) history = {};
        family.mix_history = {};
    }

    template <typename Family>
    static void promote_initial_pregen(Family& family) noexcept
    {
        if (!family.attached || !family.resampler) return;
        if (family.resampler->resampleMode != RSMODE_LINEAR
            || family.resampler->smpRateSrc >= family.resampler->smpRateDst)
            return;

        if (family.overflow || family.reconstruction_error || family.native_count != 1) {
            family.timing_valid = false;
            return;
        }
        family.mix_history.last = {};
        family.mix_history.next = family.native_mix[0];
        for (std::size_t lane = 0; lane < family.history.size(); ++lane) {
            family.history[lane].last = {};
            family.history[lane].next = family.native[lane][0];
        }
    }

    template <std::size_t LaneCount, typename Family>
    bool mirror_family_segment(
        Family& family,
        std::size_t outputLaneBase,
        std::size_t outputOffset,
        std::size_t outputCount,
        INT64 nativeResidualTolerance) noexcept
    {
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

        for (std::size_t lane = 0; lane < LaneCount; ++lane) {
            const auto result = foobar_vgm::source_audio::mirror_linear_segment(
                family.before,
                family.history[lane],
                family.native[lane].data(),
                family.native_count,
                m_output[outputLaneBase + lane].data() + outputOffset,
                outputCount);
            if (!result.exact || result.native_consumed != family.native_count) {
                family.timing_valid = false;
                return false;
            }
        }

        const INT64 tolerance_l = output_residual_tolerance(
            nativeResidualTolerance, family.before.volumeL, LaneCount);
        const INT64 tolerance_r = output_residual_tolerance(
            nativeResidualTolerance, family.before.volumeR, LaneCount);
        for (std::size_t frame = 0; frame < outputCount; ++frame) {
            INT64 sum_l = 0;
            INT64 sum_r = 0;
            for (std::size_t lane = 0; lane < LaneCount; ++lane) {
                const auto& sample = m_output[outputLaneBase + lane][outputOffset + frame];
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

    void force_genesis_source_options()
    {
        for (UINT8 instance = 0; instance < 2; ++instance) {
            const UINT32 ym_id = PLR_DEV_ID(DEVID_YM2612, instance);
            PLR_DEV_OPTS ym{};
            if (VGMPlayer::GetDeviceOptions(ym_id, ym) == 0x00) {
                ym.emuCore[0] = FCC_NUKE;
                ym.resmplMode = RSMODE_LINEAR;
                VGMPlayer::SetDeviceOptions(ym_id, ym);
            }

            const UINT32 psg_id = PLR_DEV_ID(DEVID_SN76496, instance);
            PLR_DEV_OPTS psg{};
            if (VGMPlayer::GetDeviceOptions(psg_id, psg) == 0x00) {
                psg.resmplMode = RSMODE_LINEAR;
                VGMPlayer::SetDeviceOptions(psg_id, psg);
            }
        }
    }

    void reset_for_new_device_generation() noexcept
    {
        m_ym = {};
        m_psg = {};
        m_unsupported_genesis_topology = false;
        clear_output_block();
        invalidate_output_block();
    }

    void clear_output_block() noexcept { clear_output_block(kOutputCapacity); }

    void clear_output_block(std::size_t count) noexcept
    {
        if (count > kOutputCapacity) count = kOutputCapacity;
        for (auto& lane : m_output)
            std::memset(lane.data(), 0, count * sizeof(stereo_sample));
    }

    void invalidate_output_block() noexcept
    {
        m_output_count = 0;
        m_ym_block_valid = false;
        m_psg_block_valid = false;
    }

    bool m_starting = false;
    bool m_render_capacity_ok = true;
    bool m_unsupported_genesis_topology = false;
    bool m_ym_block_valid = false;
    bool m_psg_block_valid = false;
    std::size_t m_output_count = 0;

    YmCapture m_ym{};
    PsgCapture m_psg{};
    std::array<std::array<stereo_sample, kOutputCapacity>, kLaneCount> m_output{};
};
