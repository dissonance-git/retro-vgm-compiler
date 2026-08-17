#include "components/vgm/enhancement/source_family_recomposition.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>

int main() {
    using namespace gameaudio::vgm;
    using storage_type = source_family_recomposition_storage<4, 2, 8>;

    constexpr std::size_t frames = 3;
    const std::array<float, frames> reference_left{10.0f, 20.0f, 30.0f};
    const std::array<float, frames> reference_right{-10.0f, -20.0f, -30.0f};

    const std::array<float, frames> ref0_l{1.0f, 2.0f, 3.0f};
    const std::array<float, frames> ref0_r{-1.0f, -2.0f, -3.0f};
    const std::array<float, frames> enh0_l{2.0f, 4.0f, 6.0f};
    const std::array<float, frames> enh0_r{-2.0f, -4.0f, -6.0f};

    const std::array<float, frames> ref2_l{4.0f, 4.0f, 4.0f};
    const std::array<float, frames> ref2_r{1.0f, 1.0f, 1.0f};
    const std::array<float, frames> enh2_l{8.0f, 8.0f, 8.0f};
    const std::array<float, frames> enh2_r{2.0f, 2.0f, 2.0f};

    storage_type::source_array sources{};
    sources[0].reference = {ref0_l.data(), ref0_r.data(), true};
    sources[0].enhanced = {enh0_l.data(), enh0_r.data(), true};
    sources[0].replace = true;
    sources[2].reference = {ref2_l.data(), ref2_r.data(), true};
    sources[2].enhanced = {enh2_l.data(), enh2_r.data(), true};
    sources[2].replace = true;

    const storage_type::family_map families{0, 0, 1, 1};
    storage_type storage;
    assert(storage.build_independent_families(
        reference_left.data(), reference_right.data(), frames, sources, families));
    assert(storage.valid());
    assert(storage.used_replacement());
    assert(!storage.had_family_failure());
    assert(storage.family_status(0).requested && storage.family_status(0).applied);
    assert(storage.family_status(1).requested && storage.family_status(1).applied);
    assert(std::fabs(storage.left()[0] - 15.0f) < 1e-6f);
    assert(std::fabs(storage.right()[0] + 10.0f) < 1e-6f);

    auto broken = sources;
    broken[2].enhanced.exact = false;
    assert(storage.build_independent_families(
        reference_left.data(), reference_right.data(), frames, broken, families));
    assert(storage.valid());
    assert(storage.used_replacement());
    assert(storage.had_family_failure());
    assert(storage.family_status(0).applied);
    assert(storage.family_status(1).requested && !storage.family_status(1).applied);
    assert(storage.family_status(1).error ==
        source_family_recomposition_error::missing_enhanced_source);
    assert(std::fabs(storage.left()[0] - 11.0f) < 1e-6f);
    assert(std::fabs(storage.right()[0] + 11.0f) < 1e-6f);

    assert(!storage.build(
        reference_left.data(), reference_right.data(), frames, broken));
    assert(!storage.valid());
    assert(!storage.used_replacement());
    assert(storage.last_error() == source_family_recomposition_error::missing_enhanced_source);
    assert(std::fabs(storage.left()[0] - reference_left[0]) < 1e-6f);
    assert(std::fabs(storage.right()[0] - reference_right[0]) < 1e-6f);

    auto bad_families = families;
    bad_families[3] = 2;
    assert(!storage.build_independent_families(
        reference_left.data(), reference_right.data(), frames, sources, bad_families));
    assert(storage.last_error() == source_family_recomposition_error::invalid_family_map);

    auto nonfinite_reference = reference_left;
    nonfinite_reference[1] = std::numeric_limits<float>::quiet_NaN();
    assert(!storage.build(
        nonfinite_reference.data(), reference_right.data(), frames, sources));
    assert(storage.last_error() == source_family_recomposition_error::nonfinite_sample);

    return 0;
}
