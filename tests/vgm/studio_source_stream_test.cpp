#include "components/vgm/foo_input_vgm/src/studio_source_stream.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

using namespace foobar_vgm::source_audio;

namespace {

studio_stereo_sample make_sample(std::uint64_t ordinal) {
    const double x = static_cast<double>(ordinal);
    return {
        std::sin(x * 0.071) + 0.1 * std::sin(x * 0.193),
        std::cos(x * 0.053) - 0.07 * std::cos(x * 0.217)
    };
}

studio_source_phase_position phase_at(std::uint64_t frame) {
    const std::uint64_t whole = 64 + frame;
    const std::uint64_t fraction =
        (frame * 977u) % studio_source_resampler_kernel::phase_count;
    return {
        static_cast<std::int64_t>(
            whole * studio_source_resampler_kernel::phase_count + fraction),
        true
    };
}

} // namespace

int main() {
    studio_source_resampler_kernel kernel;
    assert(kernel.configure(53267.0, 44100.0));

    constexpr std::size_t source_count = 512;
    std::array<studio_stereo_sample, source_count> source{};
    for (std::size_t i = 0; i < source.size(); ++i)
        source[i] = make_sample(i);

    studio_source_stream<1024> whole;
    assert(whole.append(0, source.data(), source.size()));

    // Same native stream arriving in irregular chunks must reconstruct exactly
    // the same Studio samples. Host/decode block boundaries are not musical time.
    studio_source_stream<1024> chunked;
    constexpr std::array<std::size_t, 9> chunks{{3, 17, 1, 64, 5, 31, 79, 2, 310}};
    std::size_t cursor = 0;
    for (std::size_t chunk : chunks) {
        assert(cursor + chunk <= source.size());
        assert(chunked.append(
            static_cast<std::uint64_t>(cursor),
            source.data() + cursor,
            chunk));
        cursor += chunk;
    }
    assert(cursor == source.size());

    for (std::uint64_t frame = 0; frame < 300; ++frame) {
        const auto position = phase_at(frame);
        const auto a = whole.reconstruct(kernel, position);
        const auto b = chunked.reconstruct(kernel, position);
        assert(a.valid && b.valid);
        assert(std::abs(a.sample.left - b.sample.left) < 1.0e-12);
        assert(std::abs(a.sample.right - b.sample.right) < 1.0e-12);
    }

    // A future-dependent destination instant remains unavailable until the final
    // native support sample exists, then becomes valid without changing phase.
    studio_source_stream<128> staged;
    std::array<studio_stereo_sample, 128> staged_source{};
    for (std::size_t i = 0; i < staged_source.size(); ++i)
        staged_source[i] = make_sample(i);
    const studio_source_phase_position target{
        static_cast<std::int64_t>(
            64 * studio_source_resampler_kernel::phase_count + 1234),
        true
    };
    const auto target_window = plan_studio_source_window(target);
    assert(target_window.valid);
    const std::size_t before_final = static_cast<std::size_t>(target_window.final);
    assert(staged.append(0, staged_source.data(), before_final));
    assert(!staged.reconstruct(kernel, target).valid);
    assert(staged.append(
        static_cast<std::uint64_t>(before_final),
        staged_source.data() + before_final,
        1));
    assert(staged.reconstruct(kernel, target).valid);

    // Discarding old history is explicit and bounded. Once discarded, an old
    // destination must fail rather than synthesizing hidden zeros.
    const auto retained_target = phase_at(100);
    assert(chunked.reconstruct(kernel, retained_target).valid);
    const auto retained_window = plan_studio_source_window(retained_target);
    assert(retained_window.valid);
    assert(chunked.discard_before(
        static_cast<std::uint64_t>(retained_window.first + 1)));
    assert(!chunked.reconstruct(kernel, retained_target).valid);

    // Gaps and overlaps are evidence corruption.
    studio_source_stream<128> discontinuous;
    assert(discontinuous.append(10, source.data() + 10, 8));
    assert(!discontinuous.append(19, source.data() + 19, 1));
    assert(!discontinuous.valid());
    discontinuous.reset();
    assert(discontinuous.valid());
    assert(discontinuous.append(10, source.data() + 10, 8));
    assert(!discontinuous.append(17, source.data() + 17, 1));
    assert(!discontinuous.valid());

    // Capacity overflow fails closed and reset is a clean seek/restart boundary.
    studio_source_stream<64> bounded;
    assert(bounded.append(0, source.data(), 64));
    assert(!bounded.append(64, source.data() + 64, 1));
    assert(!bounded.valid());
    bounded.reset();
    assert(bounded.valid());
    assert(bounded.size() == 0);

    return 0;
}
