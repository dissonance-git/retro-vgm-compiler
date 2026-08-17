#include "../../../components/vgm/foo_input_vgm/src/linear_source_resampler.h"

#include <array>
#include <cstddef>
#include <cstdint>

using foobar_vgm::source_audio::linear_history;
using foobar_vgm::source_audio::mirror_linear_segment;
using foobar_vgm::source_audio::stereo_sample;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

struct fake_stream {
    static constexpr std::size_t capture_capacity = 65536;

    std::uint32_t generation = 1;
    std::uint32_t cursor = 0;
    std::size_t captured_count = 0;
    bool overflow = false;
    std::array<stereo_sample, capture_capacity> captured{};

    void clear_capture() noexcept {
        captured_count = 0;
        overflow = false;
    }

    void reset_core() noexcept {
        ++generation;
        cursor = 0;
        clear_capture();
    }
};

stereo_sample sample_for(std::uint32_t generation, std::uint32_t index) noexcept {
    // Keep values small enough that every tested interpolation and 8.8 volume
    // multiplication remains comfortably inside DEV_SMPL while making a reset
    // generation unmistakably different from the pregenerated sample.
    const auto base = static_cast<DEV_SMPL>(generation * 100003u + index * 97u);
    return {
        static_cast<DEV_SMPL>(base - 37001),
        static_cast<DEV_SMPL>(19007 - base / 3),
    };
}

void fake_update(void* opaque, UINT32 samples, DEV_SMPL** outputs) {
    auto* stream = static_cast<fake_stream*>(opaque);
    if (!stream || !outputs || !outputs[0] || !outputs[1]) return;

    for (UINT32 i = 0; i < samples; ++i) {
        const stereo_sample value = sample_for(stream->generation, stream->cursor++);
        outputs[0][i] = value.left;
        outputs[1][i] = value.right;

        if (stream->captured_count < stream->captured.size()) {
            stream->captured[stream->captured_count++] = value;
        } else {
            stream->overflow = true;
        }
    }
}

int run_case(UINT32 source_rate, UINT32 output_rate, UINT16 volume) {
    fake_stream stream{};
    RESMPL_STATE reference{};
    reference.smpRateSrc = source_rate;
    reference.smpRateDst = output_rate;
    reference.volumeL = static_cast<INT16>(volume);
    reference.volumeR = static_cast<INT16>(volume);
    reference.resampleMode = RSMODE_LINEAR;
    reference.StreamUpdate = &fake_update;
    reference.su_DataPtr = &stream;

    Resmpl_Init(&reference);

    linear_history mirror_history{};
    if (source_rate < output_rate) {
        // libvgm's startup contract is unusual but authoritative here:
        // Resmpl_Init() consumes one native sample, then VGMPlayer::Start()
        // resets the chip without resetting the resampler's nSmpl history.
        CHECK(stream.captured_count == 1);
        mirror_history.last = {};
        mirror_history.next = stream.captured[0];
        CHECK(reference.nSmpl.L == mirror_history.next.left);
        CHECK(reference.nSmpl.R == mirror_history.next.right);
    } else {
        CHECK(stream.captured_count == 0);
    }

    // Model VGMPlayer::Start() -> Reset(): chip state starts over in a visibly
    // different generation while reference and mirror retain the same pre-roll.
    stream.reset_core();

    constexpr std::array<UINT32, 8> segments = {1, 2, 3, 7, 17, 1, 64, 5};
    std::array<WAVE_32BS, 64> reference_output{};
    std::array<stereo_sample, 64> mirrored_output{};

    for (const UINT32 frames : segments) {
        CHECK(frames <= reference_output.size());
        for (std::size_t i = 0; i < frames; ++i) {
            reference_output[i] = {};
            mirrored_output[i] = {};
        }

        const RESMPL_STATE before = reference;
        stream.clear_capture();
        Resmpl_Execute(&reference, frames, reference_output.data());
        CHECK(!stream.overflow);

        const auto mirrored = mirror_linear_segment(
            before,
            mirror_history,
            stream.captured.data(),
            stream.captured_count,
            mirrored_output.data(),
            frames);
        CHECK(mirrored.exact);
        CHECK(mirrored.native_consumed == stream.captured_count);

        for (std::size_t i = 0; i < frames; ++i) {
            CHECK(reference_output[i].L == mirrored_output[i].left);
            CHECK(reference_output[i].R == mirrored_output[i].right);
        }
    }

    Resmpl_Deinit(&reference);
    return 0;
}

} // namespace

int main() {
    // Linear-up exercises one-native-sample pre-roll retained across chip reset.
    CHECK(run_case(32000, 48000, 0x100) == 0);
    CHECK(run_case(44100, 96000, 0x0c0) == 0);

    // Copy and linear-down pin the other source-aware paths and verify that
    // command-boundary segmentation does not alter the mirror.
    CHECK(run_case(44100, 44100, 0x100) == 0);
    CHECK(run_case(53267, 44100, 0x100) == 0);
    CHECK(run_case(96000, 48000, 0x080) == 0);

    return 0;
}
