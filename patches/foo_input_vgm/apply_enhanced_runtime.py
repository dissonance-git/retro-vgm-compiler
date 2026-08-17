#!/usr/bin/env python3
"""Wire the first audible source-native Enhanced VGM replacement.

Prerequisites:
  * patches/libvgm/apply_source_capture.py was applied before libvgm build;
  * patches/foo_input_vgm/apply_enhanced_ui.py was applied to this source tree;
  * patches/foo_input_vgm/apply_source_aware_player.py was applied to this tree.

The first admitted source family is the default MAME SN76496/SN76489 path. The
protected PlayerA buffer is changed only when SourceAwareVGMPlayer proves an
exact source block and the enhanced PSG renderer can produce a complete aligned
replacement. YM2612/DAC and unsupported chips remain in the protected reference
mix until their own replacement path is independently validated.
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
    root = args.source_dir.resolve()
    header = root / "input_vgm.h"
    shadow = root / "input_vgm_shadow.cpp"
    source_player = root / "source_aware_vgm_player.h"
    cfg_external = root / "my_cfg_external.h"

    # The UI patch owns the cfg object. Tag that exact source tree so runtime
    # code can distinguish it from an upstream checkout where the symbol does
    # not exist.
    replace_once(
        cfg_external,
        """extern cfg_int cfg_vgm_enhanced_enabled;
""",
        """extern cfg_int cfg_vgm_enhanced_enabled;
#define FOO_INPUT_VGM_GAMEAUDIO_ENHANCED_UI_ABI 1
""",
        "Enhanced UI ABI tag",
    )

    # Expose only the exact outer libvgm device volume that the enhanced PSG
    # renderer needs to land in the same pre-PlayerA source domain as the exact
    # reference source lanes.
    replace_once(
        source_player,
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

    bool psg_source_volume(INT16& left, INT16& right) const noexcept
    {
        if (!m_psg.attached || m_psg.resampler == nullptr)
            return false;
        left = m_psg.resampler->volumeL;
        right = m_psg.resampler->volumeR;
        return true;
    }

protected:
""",
        "source-aware PSG device-volume view",
    )

    replace_once(
        header,
        """#include "../../enhancement/sn76489_enhanced.h"
""",
        """#include "../../enhancement/sn76489_enhanced.h"
#include "../../enhancement/sn76489_enhanced_source_block.h"
""",
        "enhanced PSG source-block include",
    )

    replace_once(
        header,
        """\tstd::array<gameaudio::vgm::sn76489_enhanced, 2> m_enhanced_psg;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
""",
        """\tstd::array<gameaudio::vgm::sn76489_enhanced, 2> m_enhanced_psg;
\tgameaudio::vgm::sn76489_enhanced_source_block_storage<8192> m_enhanced_psg_source_block;
\tstd::array<WAVE_32BS, 8192> m_enhanced_candidate_mix{};
\tbool m_enhanced_psg_block_rendered = false;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
""",
        "enhanced PSG replacement storage",
    )

    replace_once(
        header,
        """#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
\tstatic void qsound_mix_callback(void* user_param, const VGM_QSOUND_MIX_FRAME* event);
#endif
\tstatic void source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept;
""",
        """#ifdef LIBVGM_GAMEAUDIO_QSOUND_MIX_OBSERVER
\tstatic void qsound_mix_callback(void* user_param, const VGM_QSOUND_MIX_FRAME* event);
#endif
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
\tstatic void enhanced_post_render_callback(void* user_param, WAVE_32BS* samples, UINT32 sample_count, UINT32 base_playback_sample);
\tvoid apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept;
#endif
\tstatic void source_event_tap(void* user_param, const gameaudio::vgm::command_event& event) noexcept;
""",
        "enhanced PlayerA callback declarations",
    )

    # Add the transactional PlayerA callback before decode_run. Rendering occurs
    # after the ordinary VGMPlayer has completed the current block, so command
    # capture and exact source lanes already describe that same block.
    replace_once(
        shadow,
        """bool input_vgm::decode_run(audio_chunk& p_chunk, abort_callback& p_abort)
{
""",
        r'''#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
void input_vgm::enhanced_post_render_callback(
    void* user_param,
    WAVE_32BS* samples,
    UINT32 sample_count,
    UINT32)
{
    if (user_param == nullptr || samples == nullptr || sample_count == 0)
        return;
    static_cast<input_vgm*>(user_param)->apply_enhanced_post_render(samples, sample_count);
}

void input_vgm::apply_enhanced_post_render(WAVE_32BS* samples, UINT32 sample_count) noexcept
{
#if defined(FOO_INPUT_VGM_GAMEAUDIO_ENHANCED_UI_ABI)
    if (!cfg_vgm_enhanced_enabled || samples == nullptr || sample_count == 0
        || sample_count > m_enhanced_candidate_mix.size()
        || !m_shadow_configured || !m_psg_present[0] || !m_psg_shadow_valid[0])
        return;

    auto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
    if (source_player == nullptr || !source_player->psg_source_expected()
        || !source_player->psg_source_block_valid()
        || !source_player->source_block_complete()
        || source_player->source_output_count() != sample_count)
        return;

    INT16 device_volume_left = 0;
    INT16 device_volume_right = 0;
    if (!source_player->psg_source_volume(device_volume_left, device_volume_right))
        return;

    // Work on a copy. If rendering or accounting fails, the live enhanced PSG
    // state and protected PlayerA buffer remain untouched and ordinary shadow
    // replay advances the state after decode_run instead.
    auto candidate_psg = m_enhanced_psg[0];
    if (!m_enhanced_psg_source_block.render(
            candidate_psg,
            m_psg_capture,
            0,
            static_cast<std::size_t>(sample_count),
            device_volume_left,
            device_volume_right))
        return;

    for (UINT32 frame = 0; frame < sample_count; ++frame) {
        std::int64_t left = samples[frame].L;
        std::int64_t right = samples[frame].R;

        for (std::size_t channel = 0; channel < gameaudio::vgm::sn76489_enhanced::stem_count; ++channel) {
            const auto exact_lane = static_cast<SourceAwareVGMPlayer::source_lane>(
                static_cast<std::uint8_t>(SourceAwareVGMPlayer::source_lane::sn76489_tone0)
                + static_cast<std::uint8_t>(channel));
            const auto* exact = source_player->source_output(exact_lane);
            const auto enhanced = m_enhanced_psg_source_block.source(channel);
            if (exact == nullptr || enhanced.left == nullptr || enhanced.right == nullptr)
                return;

            const double enhanced_left = enhanced.left[frame];
            const double enhanced_right = enhanced.right[frame];
            if (!std::isfinite(enhanced_left) || !std::isfinite(enhanced_right))
                return;

            const auto rounded_left = static_cast<std::int64_t>(std::llround(enhanced_left));
            const auto rounded_right = static_cast<std::int64_t>(std::llround(enhanced_right));
            left += rounded_left - static_cast<std::int64_t>(exact[frame].left);
            right += rounded_right - static_cast<std::int64_t>(exact[frame].right);
        }

        if (left < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
            || left > static_cast<std::int64_t>(std::numeric_limits<INT32>::max())
            || right < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
            || right > static_cast<std::int64_t>(std::numeric_limits<INT32>::max()))
            return;

        m_enhanced_candidate_mix[frame].L = static_cast<INT32>(left);
        m_enhanced_candidate_mix[frame].R = static_cast<INT32>(right);
    }

    // Commit only after every frame and every source passed. This prevents a
    // half-enhanced block and keeps the protected reference as the fallback.
    for (UINT32 frame = 0; frame < sample_count; ++frame)
        samples[frame] = m_enhanced_candidate_mix[frame];
    m_enhanced_psg[0] = candidate_psg;
    m_enhanced_psg_block_rendered = true;
#else
    (void)samples;
    (void)sample_count;
#endif
}
#endif

bool input_vgm::decode_run(audio_chunk& p_chunk, abort_callback& p_abort)
{
''',
        "enhanced PlayerA source replacement",
    )

    replace_once(
        shadow,
        """bool input_vgm::decode_run(audio_chunk& p_chunk, abort_callback& p_abort)
{
\tm_psg_capture.reset();
""",
        """bool input_vgm::decode_run(audio_chunk& p_chunk, abort_callback& p_abort)
{
\tm_enhanced_psg_block_rendered = false;
#if defined(LIBVGM_GAMEAUDIO_SOURCE_CAPTURE_ABI) && defined(LIBVGM_GAMEAUDIO_POSTRENDER_ABI)
\tm_main_player.SetPostRenderProcessor(&input_vgm::enhanced_post_render_callback, this);
#endif
\tm_psg_capture.reset();
""",
        "PlayerA enhanced callback activation",
    )

    replace_once(
        shadow,
        """\t\tif (m_psg_capture.overflowed(instance))
\t\t\tm_psg_shadow_valid[instance] = false;
\t\telse
\t\t\tm_enhanced_psg[instance].render_timed(
\t\t\t\t\tnullptr,
\t\t\t\t\trendered_samples,
\t\t\t\t\tm_psg_capture.writes(instance),
\t\t\t\t\tm_psg_capture.count(instance));
""",
        """\t\tif (m_psg_capture.overflowed(instance))
\t\t\tm_psg_shadow_valid[instance] = false;
\t\telse if (!(instance == 0 && m_enhanced_psg_block_rendered))
\t\t\tm_enhanced_psg[instance].render_timed(
\t\t\t\t\tnullptr,
\t\t\t\t\trendered_samples,
\t\t\t\t\tm_psg_capture.writes(instance),
\t\t\t\t\tm_psg_capture.count(instance));
""",
        "avoid double-advancing enhanced PSG state",
    )

    print("foo_input_vgm first audible Enhanced PSG replacement applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
