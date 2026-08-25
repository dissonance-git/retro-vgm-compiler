#!/usr/bin/env python3
"""Final guarded repairs found by the assertion-live full core suite."""

from pathlib import Path


def replace_once(path_s: str, old: str, new: str, label: str) -> None:
    path = Path(path_s)
    text = path.read_text(encoding="utf-8")
    if old in text:
        if text.count(old) != 1:
            raise SystemExit(f"{label}: old anchor is not singular")
        path.write_text(text.replace(old, new, 1), encoding="utf-8")
        return
    if new not in text:
        raise SystemExit(f"{label}: neither old nor new anchor found")


def main() -> int:
    replace_once(
        "tests/model/test_cube_evidence_worlds.py",
        '''            {
                "work_level_single_composer_validation",
                "derivative_inheritance_candidates",
                "team_or_partial_participation",
                "arrangement_or_implementation_only",
                "future_acquisition_or_verification",
            },''',
        '''            {
                "work_level_single_composer_validation",
                "prospective_exact_control_worlds",
                "derivative_inheritance_candidates",
                "team_or_partial_participation",
                "arrangement_or_implementation_only",
                "future_acquisition_or_verification",
            },''',
        "cube evidence-world schema set",
    )

    replace_once(
        "components/vgm/enhancement/sn76489_enhanced.h",
        "    std::array<std::uint16_t, 3> tone_periods_{{1, 1, 1}};",
        "    // Preserve the raw 10-bit register across latch/data writes. Playback\n    // normalization is a separate view: an encoded zero is Sega period 1 or\n    // non-Sega period 0x400, but that normalized value must never leak back\n    // into the second half of an in-flight register write.\n    std::array<std::uint16_t, 3> tone_period_registers_{{0, 0, 0}};\n    std::array<std::uint16_t, 3> tone_periods_{{1, 1, 1}};",
        "PSG raw tone register storage",
    )

    replace_once(
        "components/vgm/enhancement/sn76489_enhanced.cpp",
        "    tone_periods_ = {{initial_period, initial_period, initial_period}};",
        "    tone_period_registers_ = {{0, 0, 0}};\n    tone_periods_ = {{initial_period, initial_period, initial_period}};",
        "PSG raw tone register reset",
    )

    replace_once(
        "components/vgm/enhancement/sn76489_enhanced.cpp",
        '''        } else if (latched_channel_ < 3) {
            auto& period = tone_periods_[latched_channel_];
            const std::uint16_t raw = static_cast<std::uint16_t>((period & 0x03F0u) | (data & 0x0Fu));
            period = normalized_period(static_cast<std::uint16_t>(raw & maximum_written_period));
        } else {''',
        '''        } else if (latched_channel_ < 3) {
            auto& raw_period = tone_period_registers_[latched_channel_];
            raw_period = static_cast<std::uint16_t>(
                ((raw_period & 0x03F0u) | (data & 0x0Fu)) & maximum_written_period);
            tone_periods_[latched_channel_] = normalized_period(raw_period);
        } else {''',
        "PSG latch-write raw period preservation",
    )

    replace_once(
        "components/vgm/enhancement/sn76489_enhanced.cpp",
        '''    } else if (latched_channel_ < 3) {
        auto& period = tone_periods_[latched_channel_];
        const std::uint16_t low = static_cast<std::uint16_t>(period & 0x000Fu);
        const std::uint16_t raw = static_cast<std::uint16_t>(low | ((data & 0x3Fu) << 4));
        period = normalized_period(static_cast<std::uint16_t>(raw & maximum_written_period));
    } else {''',
        '''    } else if (latched_channel_ < 3) {
        auto& raw_period = tone_period_registers_[latched_channel_];
        const std::uint16_t low = static_cast<std::uint16_t>(raw_period & 0x000Fu);
        raw_period = static_cast<std::uint16_t>(
            (low | ((data & 0x3Fu) << 4)) & maximum_written_period);
        tone_periods_[latched_channel_] = normalized_period(raw_period);
    } else {''',
        "PSG data-write raw period preservation",
    )

    replace_once(
        "tests/vgm/sn76489_enhanced_test.cpp",
        '''    CHECK(psg.tone_period(0) == 1);
    CHECK(psg.attenuation(0) == 15);
    CHECK(psg.noise_lfsr() == 0x8000);

    // One audible tone must remain isolated from the other three source stems.''',
        '''    CHECK(psg.tone_period(0) == 1);
    CHECK(psg.attenuation(0) == 15);
    CHECK(psg.noise_lfsr() == 0x8000);

    // A two-byte period write must preserve a zero low nibble between the
    // latch byte and following data byte. Normalizing the transient zero to 1
    // would silently turn encoded period 0x010 into 0x011.
    set_tone_period(psg, 0, 0x010);
    CHECK(psg.tone_period(0) == 0x010);

    // One audible tone must remain isolated from the other three source stems.''',
        "PSG period-16 regression",
    )

    print("final core repairs v2 applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
