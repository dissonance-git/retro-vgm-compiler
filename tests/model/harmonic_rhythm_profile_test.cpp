#include "model/harmonic_rhythm_profile.h"

#include <cmath>
#include <cstdint>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

tertian_triad_hypothesis chord(
    std::int64_t tick,
    std::int64_t root,
    tertian_triad_quality quality,
    double confidence = 0.90) {
    tertian_triad_hypothesis result;
    result.root_pitch_class = root;
    result.quality = quality;
    result.confidence = confidence;
    result.projection.source_verticality.observation_time = {
        time_domain::source,
        tick,
        0,
        0,
    };
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    // A repeated observation of the same harmony is not a harmonic change.
    const auto first = make_harmonic_rhythm_profile({
        chord(0, 0, tertian_triad_quality::major, 0.94),
        chord(50, 0, tertian_triad_quality::major, 0.92),
        chord(100, 5, tertian_triad_quality::major, 0.91),
        chord(300, 7, tertian_triad_quality::major, 0.90),
        chord(400, 0, tertian_triad_quality::major, 0.89),
    });

    CHECK(first.changes.size() == 4);
    CHECK(first.change_gaps_ticks.size() == 3);
    CHECK(first.change_gaps_ticks[0] == 100);
    CHECK(first.change_gaps_ticks[1] == 200);
    CHECK(first.change_gaps_ticks[2] == 100);
    CHECK(close_enough(first.normalization_gap_ticks, 100.0));
    CHECK(close_enough(first.normalized_change_gaps[0], 1.0));
    CHECK(close_enough(first.normalized_change_gaps[1], 2.0));
    CHECK(close_enough(first.normalized_change_gaps[2], 1.0));
    CHECK(close_enough(first.confidence, 0.89));

    // The same harmonic rhythm at exactly half the tempo retains the same
    // normalized profile. Absolute device/source ticks remain separate.
    const auto scaled = make_harmonic_rhythm_profile({
        chord(0, 0, tertian_triad_quality::major),
        chord(200, 5, tertian_triad_quality::major),
        chord(600, 7, tertian_triad_quality::major),
        chord(800, 0, tertian_triad_quality::major),
    });
    CHECK(close_enough(scaled.normalization_gap_ticks, 200.0));
    CHECK(close_enough(harmonic_rhythm_similarity(first, scaled), 1.0));

    // A different placement pattern does not receive a high similarity merely
    // because it has the same number of chord changes.
    const auto different = make_harmonic_rhythm_profile({
        chord(0, 0, tertian_triad_quality::major),
        chord(100, 5, tertian_triad_quality::major),
        chord(200, 7, tertian_triad_quality::major),
        chord(400, 0, tertian_triad_quality::major),
    });
    CHECK(harmonic_rhythm_similarity(first, different) < 0.50);

    return 0;
}
