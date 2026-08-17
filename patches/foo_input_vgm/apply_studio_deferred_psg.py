#!/usr/bin/env python3
"""Carry the Enhanced SN76489 descendant on PlayerA's engine clock.

Run after apply_studio_hq_fm_runtime.py. Studio FM render-ahead deliberately
bypasses the older decode-block PSG capture because rendered engine time can run
ahead of delivered foobar time. This patch restores Enhanced PSG without
reintroducing that clock mismatch:

* seed a private PSG synth copy from the continuous shadow when Studio engages;
* before each direct command write, render that private synth to the command's
  absolute engine-sample ordinal;
* at each deferred PlayerA callback, render the remaining command-free interval
  to the callback's rendered end;
* replace the exact four-channel PSG contribution inside the protected frame;
* then let the existing whole-frame Studio transport replace only FM.

The private queue is bounded and ordinal-checked. PSG failure is family-local and
falls back to the protected reference PSG while Studio FM may remain active.
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

    replace_once(
        header,
        '#include "../../enhancement/sn76489_enhanced_source_block.h"\n',
        '#include "../../enhancement/sn76489_enhanced_source_block.h"\n'
        '#include "../../enhancement/sn76489_deferred_source_queue.h"\n',
        "deferred PSG queue include",
    )

    replace_once(
        header,
        """\tbool m_studio_deferred_capture_bypass = false;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
""",
        """\tbool m_studio_deferred_capture_bypass = false;
\tusing studio_psg_queue_type = gameaudio::vgm::sn76489_deferred_source_queue<16640>;
\tstudio_psg_queue_type m_studio_deferred_psg_queue{};
\tgameaudio::vgm::sn76489_enhanced m_studio_deferred_psg_synth{};
\tbool m_studio_deferred_psg_active = false;
\tbool m_studio_deferred_psg_failed = false;
\tstd::array<gameaudio::vgm::ym2612_dac_enhanced, 2> m_enhanced_dac;
""",
        "deferred PSG runtime state",
    )

    replace_once(
        header,
        """\tvoid fail_studio_deferred_quality() noexcept;
#endif
""",
        """\tvoid fail_studio_deferred_quality() noexcept;
\tvoid fail_studio_deferred_psg() noexcept;
\tbool advance_studio_deferred_psg_to(std::uint64_t absolute_sample) noexcept;
#endif
""",
        "deferred PSG method declarations",
    )

    helper = r'''void input_vgm::fail_studio_deferred_psg() noexcept
{
	m_studio_deferred_psg_active = false;
	m_studio_deferred_psg_failed = true;
	m_studio_deferred_psg_queue.fail_closed();
}

bool input_vgm::advance_studio_deferred_psg_to(std::uint64_t absolute_sample) noexcept
{
	if (!m_studio_deferred_psg_active)
		return false;
	auto* source_player = static_cast<SourceAwareVGMPlayer*>(m_vgm_player);
	INT16 volume_left = 0;
	INT16 volume_right = 0;
	if (source_player == nullptr
		|| !source_player->psg_source_volume(volume_left, volume_right)
		|| !m_studio_deferred_psg_queue.render_until(
			m_studio_deferred_psg_synth,
			absolute_sample,
			volume_left,
			volume_right))
	{
		fail_studio_deferred_psg();
		return false;
	}
	return true;
}

'''

    replace_once(
        shadow,
        """void input_vgm::fail_studio_deferred_quality() noexcept
{
\tm_studio_deferred_active = false;
\tm_studio_deferred_failed = true;
\tif (m_studio_fm_transport.valid())
\t\tm_studio_fm_transport.fail_closed_reference();
}

UINT8 input_vgm::studio_deferred_post_render_callback(
""",
        """void input_vgm::fail_studio_deferred_quality() noexcept
{
\tm_studio_deferred_active = false;
\tm_studio_deferred_failed = true;
\tif (m_studio_fm_transport.valid())
\t\tm_studio_fm_transport.fail_closed_reference();
}

""" + helper + """UINT8 input_vgm::studio_deferred_post_render_callback(
""",
        "deferred PSG helper implementation",
    )

    # Direct-shadow mode sees command events at the exact engine-sample ordinal.
    # Advance the private PSG descendant first; apply_source_event_outside_render
    # below mirrors the actual write after both shadows reach that same instant.
    replace_once(
        shadow,
        """\tself->advance_shadow_to(absolute_sample);
\tself->apply_source_event_outside_render(event);
""",
        """\tif (self->m_studio_deferred_psg_active)
\t\tself->advance_studio_deferred_psg_to(static_cast<std::uint64_t>(absolute_sample));
\tself->advance_shadow_to(absolute_sample);
\tself->apply_source_event_outside_render(event);
""",
        "advance deferred PSG at direct command boundary",
    )

    replace_once(
        shadow,
        """\t\tif (psg_write && m_psg_present[psg_instance] && m_psg_shadow_valid[psg_instance])
\t\t{
\t\t\tif (stereo_mask)
\t\t\t\tm_enhanced_psg[psg_instance].write_stereo_mask(event.payload[0]);
\t\t\telse
\t\t\t\tm_enhanced_psg[psg_instance].write(event.payload[0]);
\t\t}
""",
        """\t\tif (psg_write && m_psg_present[psg_instance] && m_psg_shadow_valid[psg_instance])
\t\t{
\t\t\tif (stereo_mask)
\t\t\t\tm_enhanced_psg[psg_instance].write_stereo_mask(event.payload[0]);
\t\t\telse
\t\t\t\tm_enhanced_psg[psg_instance].write(event.payload[0]);

\t\t\tif (psg_instance == 0 && m_studio_deferred_psg_active)
\t\t\t{
\t\t\t\tif (stereo_mask)
\t\t\t\t\tm_studio_deferred_psg_synth.write_stereo_mask(event.payload[0]);
\t\t\t\telse
\t\t\t\t\tm_studio_deferred_psg_synth.write(event.payload[0]);
\t\t\t}
\t\t}
""",
        "mirror direct PSG writes into deferred descendant",
    )

    # Build the complete PSG candidate block before any frame enters the Studio
    # queue. Popping the source queue during validation is safe: on failure the
    # candidate block is discarded wholesale and the family is failed closed.
    psg_block = r'''		bool deferred_psg_block = m_studio_deferred_psg_active
			&& rendered_count <= m_enhanced_family_scratch.size()
			&& source_player != nullptr
			&& m_psg_present[0]
			&& m_psg_shadow_valid[0]
			&& source_player->psg_source_expected()
			&& source_player->psg_source_block_valid()
			&& source_player->source_output_count() == rendered_count;
		if (deferred_psg_block && !advance_studio_deferred_psg_to(rendered_end))
			deferred_psg_block = false;

		if (deferred_psg_block)
		{
			for (UINT32 frame = 0; frame < rendered_count; ++frame)
				m_enhanced_family_scratch[frame] = rendered_samples[frame];

			for (UINT32 frame = 0; frame < rendered_count && deferred_psg_block; ++frame)
			{
				const std::uint64_t ordinal =
					static_cast<std::uint64_t>(rendered_base_playback_sample)
					+ static_cast<std::uint64_t>(frame);
				gameaudio::vgm::sn76489_deferred_source_frame enhanced{};
				if (!m_studio_deferred_psg_queue.pop_expected(ordinal, enhanced))
				{
					deferred_psg_block = false;
					break;
				}

				std::int64_t exact_left = 0;
				std::int64_t exact_right = 0;
				for (std::size_t channel = 0;
					channel < gameaudio::vgm::sn76489_enhanced::stem_count;
					++channel)
				{
					const auto exact_lane = static_cast<SourceAwareVGMPlayer::source_lane>(
						static_cast<std::uint8_t>(SourceAwareVGMPlayer::source_lane::sn76489_tone0)
						+ static_cast<std::uint8_t>(channel));
					const auto* exact = source_player->source_output(exact_lane);
					if (exact == nullptr)
					{
						deferred_psg_block = false;
						break;
					}
					exact_left += static_cast<std::int64_t>(exact[frame].left);
					exact_right += static_cast<std::int64_t>(exact[frame].right);
				}
				if (!deferred_psg_block)
					break;

				const std::int64_t left =
					static_cast<std::int64_t>(rendered_samples[frame].L)
					+ enhanced.left - exact_left;
				const std::int64_t right =
					static_cast<std::int64_t>(rendered_samples[frame].R)
					+ enhanced.right - exact_right;
				if (left < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
					|| left > static_cast<std::int64_t>(std::numeric_limits<INT32>::max())
					|| right < static_cast<std::int64_t>(std::numeric_limits<INT32>::min())
					|| right > static_cast<std::int64_t>(std::numeric_limits<INT32>::max()))
				{
					deferred_psg_block = false;
					break;
				}
				m_enhanced_family_scratch[frame].L = static_cast<INT32>(left);
				m_enhanced_family_scratch[frame].R = static_cast<INT32>(right);
			}

			if (!deferred_psg_block)
				fail_studio_deferred_psg();
		}

'''

    replace_once(
        shadow,
        """\t\tconst std::uint64_t first_studio = domain_started
\t\t\t? source_player->studio_hq_fm_first_destination_ordinal()
\t\t\t: std::numeric_limits<std::uint64_t>::max();

\t\tfor (UINT32 frame = 0; frame < rendered_count; ++frame)
""",
        """\t\tconst std::uint64_t first_studio = domain_started
\t\t\t? source_player->studio_hq_fm_first_destination_ordinal()
\t\t\t: std::numeric_limits<std::uint64_t>::max();

""" + psg_block + """\t\tfor (UINT32 frame = 0; frame < rendered_count; ++frame)
""",
        "compose engine-clock Enhanced PSG candidate block",
    )

    replace_once(
        shadow,
        """\t\t\tinput.protected_left = rendered_samples[frame].L;
\t\t\tinput.protected_right = rendered_samples[frame].R;
""",
        """\t\t\tinput.protected_left = deferred_psg_block
\t\t\t\t? m_enhanced_family_scratch[frame].L : rendered_samples[frame].L;
\t\t\tinput.protected_right = deferred_psg_block
\t\t\t\t? m_enhanced_family_scratch[frame].R : rendered_samples[frame].R;
""",
        "feed PSG-enhanced protected frames into Studio transport",
    )

    # Seed from the already-aligned continuous shadow. This makes the new timing
    # representation a continuation of the existing descendant, not a retrigger.
    replace_once(
        shadow,
        """\tif (!m_studio_deferred_engaged && studio_requested)
\t{
\t\tm_studio_fm_transport.reset();
\t\tm_studio_deferred_engaged = true;
\t\tm_studio_deferred_active = true;
\t}
""",
        """\tif (!m_studio_deferred_engaged && studio_requested)
\t{
\t\tm_studio_fm_transport.reset();
\t\tm_studio_deferred_engaged = true;
\t\tm_studio_deferred_active = true;
\t\tm_studio_deferred_psg_failed = false;
\t\tm_studio_deferred_psg_active = false;
\t\tif (m_psg_present[0] && m_psg_shadow_valid[0]
\t\t\t&& studio_source_player->psg_source_expected())
\t\t{
\t\t\tINT16 psg_volume_left = 0;
\t\t\tINT16 psg_volume_right = 0;
\t\t\tif (studio_source_player->psg_source_volume(psg_volume_left, psg_volume_right))
\t\t\t{
\t\t\t\tm_studio_deferred_psg_synth = m_enhanced_psg[0];
\t\t\t\tm_studio_deferred_psg_queue.reset(
\t\t\t\t\tstatic_cast<std::uint64_t>(m_shadow_replay_sample));
\t\t\t\tm_studio_deferred_psg_active = true;
\t\t\t}
\t\t}
\t}
""",
        "seed deferred PSG from continuous engine shadow",
    )

    replace_once(
        shadow,
        """\tif (m_studio_deferred_engaged && (!cfg_vgm_enhanced_enabled
\t\t|| m_studio_deferred_failed || studio_source_player == nullptr
\t\t|| !studio_source_player->studio_hq_fm_observer_valid()))
\t{
\t\tm_studio_deferred_active = false;
\t\tm_studio_fm_transport.fail_closed_reference();
\t}
""",
        """\tif (m_studio_deferred_engaged && (!cfg_vgm_enhanced_enabled
\t\t|| m_studio_deferred_failed || studio_source_player == nullptr
\t\t|| !studio_source_player->studio_hq_fm_observer_valid()))
\t{
\t\tm_studio_deferred_active = false;
\t\tm_studio_fm_transport.fail_closed_reference();
\t\tif (m_studio_deferred_psg_active)
\t\t\tfail_studio_deferred_psg();
\t}
""",
        "stop deferred PSG with disabled Enhanced transport",
    )

    replace_once(
        shadow,
        """\tm_studio_deferred_capture_bypass = false;
#endif
\tm_source_capture_active = false;
""",
        """\tm_studio_deferred_capture_bypass = false;
\tm_studio_deferred_psg_active = false;
\tm_studio_deferred_psg_failed = false;
\tm_studio_deferred_psg_queue.reset();
#endif
\tm_source_capture_active = false;
""",
        "reset deferred PSG on seek",
    )

    print("foo_input_vgm engine-clock deferred Enhanced PSG applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
