#!/usr/bin/env python3
from pathlib import Path

path = Path("tests/vgm/sn76489_alias_test.cpp")
text = path.read_text(encoding="utf-8")

old_include = "#include <cstdint>\n"
new_include = "#include <cstdint>\n#include <iostream>\n"
if old_include in text:
    if text.count(old_include) != 1:
        raise SystemExit("cstdint include anchor is not singular")
    text = text.replace(old_include, new_include, 1)
elif new_include not in text:
    raise SystemExit("diagnostic include anchor not found")

old_macro = "#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)"
new_macro = (
    '#define CHECK(expression) do { if (!(expression)) { '
    'std::cerr << "CHECK failed at line " << __LINE__ << ": " #expression << "\\n"; '
    'return __LINE__; } } while (false)'
)
if old_macro in text:
    if text.count(old_macro) != 1:
        raise SystemExit("CHECK macro anchor is not singular")
    text = text.replace(old_macro, new_macro, 1)
elif new_macro not in text:
    raise SystemExit("diagnostic CHECK macro anchor not found")

old_values = (
    "    const double enhanced_error = bandlimited_square_harmonic_error(enhanced);\n"
    "    const double naive_error = bandlimited_square_harmonic_error(naive);\n"
)
new_values = old_values + (
    '    std::cerr << "enhanced_error=" << enhanced_error\n'
    '              << " naive_error=" << naive_error\n'
    '              << " ratio=" << (enhanced_error / naive_error) << "\\n";\n'
)
if old_values in text:
    if text.count(old_values) != 1:
        raise SystemExit("alias metric anchor is not singular")
    text = text.replace(old_values, new_values, 1)
elif new_values not in text:
    raise SystemExit("alias metric diagnostic anchor not found")

old_fund = (
    "    const double enhanced_fundamental = sinusoid_energy(enhanced, 256);\n"
    "    const double naive_fundamental = sinusoid_energy(naive, 256);\n"
)
new_fund = old_fund + (
    '    std::cerr << "enhanced_fundamental=" << enhanced_fundamental\n'
    '              << " naive_fundamental=" << naive_fundamental\n'
    '              << " ratio=" << (enhanced_fundamental / naive_fundamental) << "\\n";\n'
)
if old_fund in text:
    if text.count(old_fund) != 1:
        raise SystemExit("fundamental metric anchor is not singular")
    text = text.replace(old_fund, new_fund, 1)
elif new_fund not in text:
    raise SystemExit("fundamental diagnostic anchor not found")

path.write_text(text, encoding="utf-8")
