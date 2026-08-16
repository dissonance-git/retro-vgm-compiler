#include "model/tonal_center_hypothesis.h"

#include <cmath>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

tonal_center_evidence support(
    tonal_center_evidence_kind kind,
    tonal_center_evidence_origin origin,
    double center,
    double confidence,
    const char* dependency) {
    tonal_center_evidence evidence;
    evidence.kind = kind;
    evidence.origin = origin;
    evidence.center_octave_class = center;
    evidence.confidence = confidence;
    evidence.dependency_group = dependency;
    return evidence;
}
} // namespace

int main() {
    const time_span region{at(0), at(1000)};

    // Octave class is circular, not a clipped linear coordinate.
    CHECK(close_enough(circular_octave_class_distance(0.99, 0.01), 0.02));
    CHECK(close_enough(normalize_octave_class(-0.01), 0.99));

    // One excellent clue is still only one clue.
    const auto one = infer_tonal_center_hypothesis(
        0.0,
        region,
        {support(
            tonal_center_evidence_kind::recurrence,
            tonal_center_evidence_origin::pitch_distribution,
            0.0,
            0.95,
            "pitch-distribution:phrase-a")});
    CHECK(one.independent_support_groups == 1);
    CHECK(one.independent_support_origins == 1);
    CHECK(close_enough(one.confidence, tonal_center_single_support_ceiling));
    CHECK(!one.cross_origin_grounded);

    // Multiple independent windows from one evidence domain may strengthen the
    // case, but cannot masquerade as cross-domain grounding.
    const auto one_origin = infer_tonal_center_hypothesis(
        0.0,
        region,
        {
            support(
                tonal_center_evidence_kind::recurrence,
                tonal_center_evidence_origin::pitch_distribution,
                0.0,
                0.93,
                "pitch-distribution:phrase-a"),
            support(
                tonal_center_evidence_kind::duration,
                tonal_center_evidence_origin::pitch_distribution,
                0.0,
                0.91,
                "pitch-distribution:phrase-b"),
            support(
                tonal_center_evidence_kind::recurrence,
                tonal_center_evidence_origin::pitch_distribution,
                0.0,
                0.89,
                "pitch-distribution:phrase-c"),
        });
    CHECK(one_origin.independent_support_groups == 3);
    CHECK(one_origin.independent_support_origins == 1);
    CHECK(close_enough(one_origin.confidence, tonal_center_single_origin_ceiling));

    // Duplicate derivatives of the same witness collapse into one dependency
    // vote. Adding a genuinely independent bass witness creates two groups.
    const auto two_domains = infer_tonal_center_hypothesis(
        0.0,
        region,
        {
            support(
                tonal_center_evidence_kind::recurrence,
                tonal_center_evidence_origin::pitch_distribution,
                0.0,
                0.94,
                "verticality:42"),
            support(
                tonal_center_evidence_kind::harmonic_stability,
                tonal_center_evidence_origin::harmony,
                0.0,
                0.92,
                "verticality:42"),
            support(
                tonal_center_evidence_kind::bass_support,
                tonal_center_evidence_origin::bass_structure,
                0.0,
                0.88,
                "bass-trajectory:7"),
        });
    CHECK(two_domains.independent_support_groups == 2);
    CHECK(two_domains.independent_support_origins == 2);
    CHECK(two_domains.cross_origin_grounded);
    CHECK(close_enough(two_domains.confidence, tonal_center_two_group_ceiling));

    // A third independent structural origin can raise the center hypothesis,
    // but still does not name a key, mode or functional tonic.
    const auto three_domains = infer_tonal_center_hypothesis(
        0.0,
        region,
        {
            support(
                tonal_center_evidence_kind::recurrence,
                tonal_center_evidence_origin::pitch_distribution,
                0.0,
                0.94,
                "pitch-distribution:whole-region"),
            support(
                tonal_center_evidence_kind::bass_support,
                tonal_center_evidence_origin::bass_structure,
                0.0,
                0.90,
                "bass-trajectory:7"),
            support(
                tonal_center_evidence_kind::structural_arrival,
                tonal_center_evidence_origin::phrase_structure,
                0.0,
                0.86,
                "phrase-arrival:1000"),
        });
    CHECK(three_domains.independent_support_groups == 3);
    CHECK(three_domains.independent_support_origins == 3);
    CHECK(close_enough(three_domains.confidence, tonal_center_three_group_ceiling));
    CHECK(!three_domains.key_named);
    CHECK(!three_domains.mode_named);
    CHECK(!three_domains.tonal_function_named);

    // Evidence for some other center is not automatically contradiction. Tonal
    // music may strongly articulate other pitch classes inside the same region.
    const auto other_center_ignored = infer_tonal_center_hypothesis(
        0.0,
        region,
        {
            support(
                tonal_center_evidence_kind::recurrence,
                tonal_center_evidence_origin::pitch_distribution,
                0.0,
                0.94,
                "pitch-distribution:whole-region"),
            support(
                tonal_center_evidence_kind::harmonic_stability,
                tonal_center_evidence_origin::harmony,
                7.0 / 12.0,
                0.99,
                "harmony:other-root"),
        });
    CHECK(other_center_ignored.independent_support_groups == 1);
    CHECK(!other_center_ignored.strong_counterevidence);
    CHECK(close_enough(other_center_ignored.confidence, tonal_center_single_support_ceiling));

    // Counterevidence must be explicit. Once it is strong and targets this same
    // center claim, the hypothesis cannot remain strong merely by accumulating
    // supporting transformations of the data.
    auto contradiction = support(
        tonal_center_evidence_kind::contradiction,
        tonal_center_evidence_origin::external_annotation,
        0.0,
        0.90,
        "independent-annotation");
    contradiction.supports_candidate = false;

    const auto conflicted = infer_tonal_center_hypothesis(
        0.0,
        region,
        {
            support(
                tonal_center_evidence_kind::recurrence,
                tonal_center_evidence_origin::pitch_distribution,
                0.0,
                0.94,
                "pitch-distribution:whole-region"),
            support(
                tonal_center_evidence_kind::bass_support,
                tonal_center_evidence_origin::bass_structure,
                0.0,
                0.91,
                "bass-trajectory:7"),
            support(
                tonal_center_evidence_kind::structural_arrival,
                tonal_center_evidence_origin::phrase_structure,
                0.0,
                0.88,
                "phrase-arrival:1000"),
            contradiction,
        });
    CHECK(conflicted.strong_counterevidence);
    CHECK(close_enough(conflicted.confidence, tonal_center_strong_conflict_ceiling));

    return 0;
}
