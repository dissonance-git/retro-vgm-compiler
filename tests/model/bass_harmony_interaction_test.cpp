#include "model/bass_harmony_interaction.h"

#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

tertian_triad_hypothesis chord(
    std::int64_t root,
    tertian_triad_quality quality,
    std::vector<std::int64_t> steps,
    std::vector<node_id> parts,
    double confidence = 0.92) {
    tertian_triad_hypothesis result;
    result.root_pitch_class = root;
    result.quality = quality;
    result.confidence = confidence;
    result.projection.tuning.divisions_per_octave = 12;
    result.projection.nearest_steps = std::move(steps);
    result.projection.source_verticality.part_ids = std::move(parts);
    return result;
}

voice_leading_hypothesis voices(
    std::vector<voice_leading_motion> motions,
    double confidence = 0.90) {
    voice_leading_hypothesis result;
    result.motions = std::move(motions);
    result.identity_preserved_voices = result.motions.size();
    result.all_correspondence_identity_grounded = true;
    result.confidence = confidence;
    for (const auto& motion : result.motions)
        result.total_absolute_motion_semitones += std::llabs(motion.semitone_motion);
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}
} // namespace

int main() {
    // C major -> A minor while the upper C/E material is retained and the
    // persistent bass line moves C -> A. This is a relational compositional
    // event, not merely two chord labels.
    const auto c_major = chord(
        0,
        tertian_triad_quality::major,
        {48, 60, 64},
        {1, 2, 3},
        0.94);
    const auto a_minor = chord(
        9,
        tertian_triad_quality::minor,
        {45, 60, 64},
        {1, 2, 3},
        0.91);
    const auto retained_upper = voices({
        {48, 45, -3, 1, 1, true},
        {60, 60, 0, 2, 2, true},
        {64, 64, 0, 3, 3, true},
    }, 0.90);

    const auto moving_bass = infer_bass_harmony_interaction(
        c_major,
        a_minor,
        retained_upper);
    CHECK(moving_bass.kind ==
        bass_harmony_interaction_kind::moving_bass_under_retained_upper_material);
    CHECK(moving_bass.bass_identity_grounded);
    CHECK(moving_bass.bass_part_id == 1);
    CHECK(moving_bass.bass_motion_semitones == -3);
    CHECK(moving_bass.retained_upper_pitch_classes == 2);
    CHECK(close_enough(moving_bass.confidence, 0.90));

    // C major -> F/C: the same bass part and pitch are held while the harmony
    // changes above it, a pedal-bass relation.
    const auto f_over_c = chord(
        5,
        tertian_triad_quality::major,
        {48, 60, 65},
        {1, 2, 3},
        0.91);
    const auto pedal_voices = voices({
        {48, 48, 0, 1, 1, true},
        {60, 60, 0, 2, 2, true},
        {64, 65, 1, 3, 3, true},
    }, 0.89);
    const auto pedal = infer_bass_harmony_interaction(c_major, f_over_c, pedal_voices);
    CHECK(pedal.kind == bass_harmony_interaction_kind::pedal_bass_under_harmonic_change);
    CHECK(pedal.bass_motion_semitones == 0);
    CHECK(pedal.retained_upper_pitch_classes == 1);
    CHECK(close_enough(pedal.confidence, 0.89));

    // A lowest physical pitch with a different part identity is not silently
    // treated as continuation of the same bass line.
    const auto replaced_bass = chord(
        9,
        tertian_triad_quality::minor,
        {45, 60, 64},
        {9, 2, 3},
        0.91);
    const auto unresolved = infer_bass_harmony_interaction(
        c_major,
        replaced_bass,
        retained_upper);
    CHECK(unresolved.kind == bass_harmony_interaction_kind::unresolved);
    CHECK(!unresolved.bass_identity_grounded);
    CHECK(close_enough(unresolved.confidence, unresolved_bass_identity_ceiling));

    return 0;
}
