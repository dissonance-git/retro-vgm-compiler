#include "../../components/vgm/enhancement/qsound_native_capture_coherence.h"

#include <array>
#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

void add_source(qsound_native_source_capture& capture, std::uint64_t sample, std::uint32_t rate = 24038) {
    std::array<std::int16_t, qsound_source_count> source{};
    capture.observe(0, rate, sample, source.data(), source.size());
}

void add_mix(qsound_native_mix_capture& capture, std::uint64_t sample, std::uint32_t rate = 24038) {
    qsound_native_mix_frame frame;
    frame.native_sample = sample;
    frame.accounting_valid = true;
    capture.observe(0, rate, &frame);
}

} // namespace

int main() {
    qsound_native_source_capture source;
    qsound_native_mix_capture mix;

    source.begin_block();
    mix.begin_block();
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::coherent);

    add_source(source, 100);
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::frame_count_mismatch);

    mix.begin_block();
    add_mix(mix, 100, 24039);
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::sample_rate_mismatch);

    mix.begin_block();
    add_mix(mix, 101);
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::first_sample_mismatch);

    mix.begin_block();
    add_mix(mix, 100);
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::coherent);

    add_source(source, 101);
    add_mix(mix, 101);
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::coherent);

    // Per-frame accounting availability does not alter the shared device clock.
    qsound_native_mix_frame unavailable;
    unavailable.native_sample = 102;
    unavailable.accounting_valid = false;
    std::array<std::int16_t, qsound_source_count> source_frame{};
    source.observe(0, 24038, 102, source_frame.data(), source_frame.size());
    mix.observe(0, 24038, &unavailable);
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::coherent);

    // Structural invalidity still fails closed before any metadata comparison.
    source.observe(0, 24038, 104, source_frame.data(), source_frame.size());
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::source_capture_invalid);

    source.begin_block();
    mix.begin_block();
    add_source(source, 200);
    add_mix(mix, 200);
    qsound_native_mix_frame gap;
    gap.native_sample = 202;
    mix.observe(0, 24038, &gap);
    CHECK(qsound_compare_native_captures(source, mix) == qsound_native_coherence::mix_capture_invalid);

    return 0;
}
