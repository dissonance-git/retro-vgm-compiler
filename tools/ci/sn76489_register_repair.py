#!/usr/bin/env python3
"""Stage the evidence-backed SN76489 raw-register repair for the full core gate."""

from pathlib import Path


def replace_once(path: str, old: str, new: str, label: str) -> None:
    target = Path(path)
    text = target.read_text(encoding="utf-8")
    if old in text:
        if text.count(old) != 1:
            raise SystemExit(f"{label}: anchor is not singular")
        target.write_text(text.replace(old, new, 1), encoding="utf-8")
        return
    if new not in text:
        raise SystemExit(f"{label}: anchor not found")


# Keep the emulated 10-bit tone register raw. Zero-period behavior is an
# oscillator/effective-period rule, not a mutation of register state between
# the low-nibble latch and following high-six-bit data write.
replace_once(
    "components/vgm/enhancement/sn76489_enhanced.cpp",
    "    const std::uint16_t initial_period = normalized_period(0);\n"
    "    tone_periods_ = {{initial_period, initial_period, initial_period}};",
    "    tone_periods_ = {{0, 0, 0}};",
    "SN76489 reset register state",
)
replace_once(
    "components/vgm/enhancement/sn76489_enhanced.cpp",
    "            const std::uint16_t raw = static_cast<std::uint16_t>((period & 0x03F0u) | (data & 0x0Fu));\n"
    "            period = normalized_period(static_cast<std::uint16_t>(raw & maximum_written_period));",
    "            const std::uint16_t raw = static_cast<std::uint16_t>((period & 0x03F0u) | (data & 0x0Fu));\n"
    "            period = static_cast<std::uint16_t>(raw & maximum_written_period);",
    "SN76489 latch write",
)
replace_once(
    "components/vgm/enhancement/sn76489_enhanced.cpp",
    "        const std::uint16_t raw = static_cast<std::uint16_t>(low | ((data & 0x3Fu) << 4));\n"
    "        period = normalized_period(static_cast<std::uint16_t>(raw & maximum_written_period));",
    "        const std::uint16_t raw = static_cast<std::uint16_t>(low | ((data & 0x3Fu) << 4));\n"
    "        period = static_cast<std::uint16_t>(raw & maximum_written_period);",
    "SN76489 continuation write",
)
replace_once(
    "components/vgm/enhancement/sn76489_enhanced.cpp",
    "    return tone_periods_[channel];",
    "    return normalized_period(tone_periods_[channel]);",
    "SN76489 effective period accessor",
)

# The exact failure case: a low-nibble-zero latch must not synthesize bit 0
# before the high six bits arrive. 0x80 followed by 0x01 means period 0x010,
# never 0x011.
replace_once(
    "tests/vgm/sn76489_enhanced_test.cpp",
    "    CHECK(psg.tone_period(0) == 1);\n"
    "    CHECK(psg.attenuation(0) == 15);",
    "    CHECK(psg.tone_period(0) == 1);\n"
    "    psg.write(0x80); // tone 0 latch, low nibble = 0\n"
    "    CHECK(psg.tone_period(0) == 1); // effective Sega zero-period behavior\n"
    "    psg.write(0x01); // high six bits = 1 => raw register 0x010\n"
    "    CHECK(psg.tone_period(0) == 0x010);\n"
    "    psg.reset();\n"
    "    CHECK(psg.attenuation(0) == 15);",
    "SN76489 split-write regression",
)

print("SN76489 raw-register repair staged")
