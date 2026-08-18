#include "components/vgm/enhancement/ym2151_native_source_witness.h"

#include <cassert>
#include <cstddef>

int main() {
    using namespace gameaudio::vgm;

    ym2151_native_source_witness_validator validator;
    validator.reset();

    ym2151_native_source_witness witness{};
    for (std::size_t channel = 0; channel < ym2151_recomposition_source_count; ++channel) {
        witness.left[channel] = static_cast<std::int32_t>(channel + 1u);
        witness.right[channel] = -static_cast<std::int32_t>(channel + 1u);
        witness.mix_left += witness.left[channel];
        witness.mix_right += witness.right[channel];
    }

    assert(validator.observe(witness));
    assert(validator.valid());
    assert(validator.sample_count() == 1u);

    // A tap that silently loses or double-counts even one channel is not exact
    // source evidence and permanently invalidates the capture generation.
    auto broken = witness;
    broken.left[3] += 1;
    assert(!validator.observe(broken));
    assert(!validator.valid());
    assert(validator.sample_count() == 1u);
    assert(validator.last_error() == ym2151_native_source_witness_error::mix_mismatch);

    // Once exact accounting is lost, later plausible samples cannot repair the
    // same generation; caller must reset/reseed deliberately.
    assert(!validator.observe(witness));
    validator.reset();
    assert(validator.observe(witness));
    assert(validator.sample_count() == 1u);
    return 0;
}
