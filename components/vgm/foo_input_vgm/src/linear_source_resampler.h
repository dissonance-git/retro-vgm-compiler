#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {
#include <emu/Resampler.h>
}

namespace foobar_vgm::source_audio {

struct stereo_sample {
    DEV_SMPL left = 0;
    DEV_SMPL right = 0;
};

// Per-source history corresponding to libvgm RESMPL_STATE::lSmpl/nSmpl.
// Timing counters are deliberately NOT duplicated here; every segment uses the
// reference device's exact pre-Resmpl_Execute timing snapshot.
struct linear_history {
    stereo_sample last{};
    stereo_sample next{};
};

struct mirror_result {
    std::size_t native_consumed = 0;
    bool exact = false;
};

namespace detail {
constexpr std::uint32_t fixed_bits = 11;
constexpr std::uint32_t fixed_factor = 1u << fixed_bits;
constexpr std::uint32_t fixed_mask = fixed_factor - 1u;
constexpr std::uint32_t storage_bits = 32;
constexpr std::uint32_t overflow_bit = storage_bits - fixed_bits;
constexpr std::uint32_t overflow_span = 1u << overflow_bit;
constexpr std::uint32_t overflow_mask = overflow_span - 1u;

inline std::uint32_t fraction(std::uint32_t x) noexcept { return x & fixed_mask; }
inline std::uint32_t nfraction(std::uint32_t x) noexcept {
    return (fixed_factor - x) & fixed_mask;
}
inline std::uint32_t floor_fp(std::uint32_t x) noexcept { return x / fixed_factor; }
inline std::uint32_t ceil_fp(std::uint32_t x) noexcept {
    return (x + fixed_mask) / fixed_factor;
}

inline stereo_sample scale(stereo_sample s, INT16 volume_l, INT16 volume_r) noexcept {
    s.left = static_cast<DEV_SMPL>(static_cast<INT64>(s.left) * volume_l);
    s.right = static_cast<DEV_SMPL>(static_cast<INT64>(s.right) * volume_r);
    return s;
}

inline stereo_sample conceptual_up_sample(
    std::size_t index,
    const linear_history& history,
    const stereo_sample* newly_captured,
    std::size_t captured_count) noexcept {
    if (index == 0) return history.last;
    if (index == 1) return history.next;
    const std::size_t native_index = index - 2;
    return newly_captured && native_index < captured_count
        ? newly_captured[native_index]
        : stereo_sample{};
}

inline stereo_sample conceptual_down_sample(
    std::size_t index,
    const linear_history& history,
    const stereo_sample* newly_captured,
    std::size_t captured_count) noexcept {
    if (index == 0) return history.last;
    const std::size_t native_index = index - 1;
    return newly_captured && native_index < captured_count
        ? newly_captured[native_index]
        : stereo_sample{};
}
} // namespace detail

// Mirrors libvgm's default RSMODE_LINEAR path for one source lane over one exact
// Resmpl_Execute segment. `before` is copied immediately before the reference
// call; `native` contains exactly the samples emitted by wrapped StreamUpdate
// calls made inside that call. No allocation, chip calls, or look-ahead.
inline mirror_result mirror_linear_segment(
    const RESMPL_STATE& before,
    linear_history& history,
    const stereo_sample* native,
    std::size_t native_count,
    stereo_sample* output,
    std::size_t output_count) noexcept {

    mirror_result result{};
    if (!output || (!native && native_count != 0) || before.resampleMode != RSMODE_LINEAR
        || before.smpRateSrc == 0 || before.smpRateDst == 0) {
        return result;
    }

    if (before.smpRateSrc == before.smpRateDst) {
        if (native_count != output_count) return result;
        for (std::size_t i = 0; i < output_count; ++i)
            output[i] = detail::scale(native[i], before.volumeL, before.volumeR);
        result.native_consumed = native_count;
        result.exact = true;
        return result;
    }

    if (before.smpRateSrc < before.smpRateDst) {
        std::uint32_t smp_p = before.smpP;
        std::uint32_t smp_last = before.smpLast;
        std::uint32_t smp_next = before.smpNext;
        const std::uint64_t chip_rate_fp =
            static_cast<std::uint64_t>(detail::fixed_factor) * before.smpRateSrc;
        std::size_t cursor = 0;

        for (std::size_t out_pos = 0; out_pos < output_count; ++out_pos) {
            const std::uint32_t in_pos_l = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(smp_p) * chip_rate_fp / before.smpRateDst);
            const std::uint32_t in_pre_abs = detail::floor_fp(in_pos_l);
            const std::uint32_t in_now_abs = detail::ceil_fp(in_pos_l);
            if (in_now_abs < smp_next) return result;
            const std::size_t needed = static_cast<std::size_t>(in_now_abs - smp_next);
            if (cursor + needed > native_count) return result;

            const std::uint32_t in_base = detail::fixed_factor
                + static_cast<std::uint32_t>(in_pos_l - smp_next * detail::fixed_factor);
            smp_last = in_pre_abs;
            smp_next = in_now_abs;

            const std::uint32_t in_pre = detail::floor_fp(in_base);
            const std::uint32_t in_now = detail::ceil_fp(in_base);
            const std::uint32_t frac = detail::fraction(in_base);
            const stereo_sample* captured = native ? native + cursor : nullptr;
            const stereo_sample a = detail::conceptual_up_sample(
                in_pre, history, captured, needed);
            const stereo_sample b = detail::conceptual_up_sample(
                in_now, history, captured, needed);

            const INT64 left = static_cast<INT64>(a.left) * (detail::fixed_factor - frac)
                + static_cast<INT64>(b.left) * frac;
            const INT64 right = static_cast<INT64>(a.right) * (detail::fixed_factor - frac)
                + static_cast<INT64>(b.right) * frac;
            output[out_pos].left = static_cast<DEV_SMPL>(
                left * before.volumeL / detail::fixed_factor);
            output[out_pos].right = static_cast<DEV_SMPL>(
                right * before.volumeR / detail::fixed_factor);

            history.last = a;
            history.next = b;
            cursor += needed;
            ++smp_p;
        }

        if (smp_last >= before.smpRateSrc) {
            smp_last -= before.smpRateSrc;
            smp_next -= before.smpRateSrc;
            smp_p -= before.smpRateDst;
        }
        (void)smp_last;
        (void)smp_next;
        (void)smp_p;

        if (cursor != native_count) return result;
        result.native_consumed = cursor;
        result.exact = true;
        return result;
    }

    std::uint32_t smp_p = before.smpP;
    const std::uint32_t smp_last = before.smpLast;
    const std::uint64_t chip_rate_fp =
        static_cast<std::uint64_t>(detail::fixed_factor) * before.smpRateSrc;

    const std::uint32_t end_pos_l = static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(smp_p + static_cast<std::uint32_t>(output_count))
        * chip_rate_fp / before.smpRateDst);
    std::uint32_t smp_next = detail::ceil_fp(end_pos_l);

    if (smp_next < smp_last) {
        smp_next |= smp_last & ~detail::overflow_mask;
        if (smp_next < smp_last)
            smp_next += detail::overflow_span;
    }
    if (smp_next < smp_last) return result;
    const std::size_t needed = static_cast<std::size_t>(smp_next - smp_last);
    if (needed != native_count) return result;

    const std::uint32_t begin_pos_l = static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(smp_p) * chip_rate_fp / before.smpRateDst);
    const std::uint32_t in_base = detail::fixed_factor
        + static_cast<std::uint32_t>(begin_pos_l - smp_last * detail::fixed_factor);
    std::uint32_t in_pos_next = in_base;
    std::uint32_t last_in_pre = detail::floor_fp(in_pos_next);

    for (std::size_t out_pos = 0; out_pos < output_count; ++out_pos) {
        const std::uint32_t in_pos = in_pos_next;
        in_pos_next = in_base + static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(out_pos + 1) * chip_rate_fp / before.smpRateDst);

        std::uint32_t frac = detail::nfraction(in_pos);
        INT64 acc_l = 0;
        INT64 acc_r = 0;
        INT32 weight = static_cast<INT32>(frac);
        if (frac) {
            const std::uint32_t idx = detail::floor_fp(in_pos);
            const auto s = detail::conceptual_down_sample(idx, history, native, native_count);
            acc_l = static_cast<INT64>(s.left) * frac;
            acc_r = static_cast<INT64>(s.right) * frac;
        }

        frac = detail::fraction(in_pos_next);
        last_in_pre = detail::floor_fp(in_pos_next);
        if (frac) {
            const auto s = detail::conceptual_down_sample(last_in_pre, history, native, native_count);
            acc_l += static_cast<INT64>(s.left) * frac;
            acc_r += static_cast<INT64>(s.right) * frac;
            weight += static_cast<INT32>(frac);
        }

        std::uint32_t in_now = detail::ceil_fp(in_pos);
        weight += static_cast<INT32>(last_in_pre - in_now)
            * static_cast<INT32>(detail::fixed_factor);
        while (in_now < last_in_pre) {
            const auto s = detail::conceptual_down_sample(in_now, history, native, native_count);
            acc_l += static_cast<INT64>(s.left) * detail::fixed_factor;
            acc_r += static_cast<INT64>(s.right) * detail::fixed_factor;
            ++in_now;
        }
        if (weight <= 0) return result;

        output[out_pos].left = static_cast<DEV_SMPL>(acc_l * before.volumeL / weight);
        output[out_pos].right = static_cast<DEV_SMPL>(acc_r * before.volumeR / weight);
    }

    history.last = detail::conceptual_down_sample(
        last_in_pre, history, native, native_count);
    smp_p += static_cast<std::uint32_t>(output_count);
    (void)smp_p;

    result.native_consumed = native_count;
    result.exact = true;
    return result;
}

} // namespace foobar_vgm::source_audio
