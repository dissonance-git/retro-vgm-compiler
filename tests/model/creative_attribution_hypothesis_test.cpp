#include "model/creative_attribution_hypothesis.h"

#include <cassert>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

namespace {

bool close_enough(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1e-9;
}

creative_attribution_evidence support(
    creative_attribution_evidence_kind kind,
    creative_attribution_role role,
    evidence_status status,
    double confidence,
    const char* source,
    const char* detail) {
    return {
        kind,
        role,
        creative_attribution_polarity::supports,
        status,
        confidence,
        source,
        detail,
    };
}

creative_attribution_evidence counter(
    creative_attribution_role role,
    double confidence,
    const char* source,
    const char* detail) {
    return {
        creative_attribution_evidence_kind::contradiction,
        role,
        creative_attribution_polarity::counters,
        evidence_status::derived,
        confidence,
        source,
        detail,
    };
}

} // namespace

int main() {
    // Sonic-inspired control 1: a strong FM/modulation/channel fingerprint can
    // support an arranger/programmer hypothesis, but it must not leak upward
    // into composer attribution. Final confidence is bounded by the weaker
    // independent grounding domain, not the caller's proposed confidence.
    std::vector<creative_attribution_evidence> realization_evidence{
        support(
            creative_attribution_evidence_kind::arrangement_execution,
            creative_attribution_role::arranger_programmer,
            evidence_status::derived,
            0.94,
            "held-out-vgm-controls",
            "FM patch, modulation and PSG deployment match the candidate realization family"),
        support(
            creative_attribution_evidence_kind::version_lineage,
            creative_attribution_role::arranger_programmer,
            evidence_status::hypothesis,
            0.82,
            "prototype-final-comparison",
            "prototype/final realization changes are compatible with the same programmer family"),
    };

    const auto programmer = make_creative_attribution_hypothesis(
        "candidate-programmer",
        creative_attribution_role::arranger_programmer,
        0.91,
        realization_evidence);
    assert(programmer.role_specific_support);
    assert(programmer.cross_domain_grounded);
    assert(programmer.support_domains == 2);
    assert(programmer.grounding_support_observations == 2);
    assert(close_enough(programmer.independent_support_ceiling, 0.82));
    assert(close_enough(programmer.confidence, 0.82));

    const auto composer_from_programming = make_creative_attribution_hypothesis(
        "candidate-programmer",
        creative_attribution_role::composer,
        0.91,
        realization_evidence);
    assert(!composer_from_programming.role_specific_support);
    assert(close_enough(
        composer_from_programming.confidence,
        creative_attribution_no_role_support_ceiling));

    // Control 2: composition-facing melody/form evidence can point to a
    // composer without becoming evidence that the same person programmed the
    // Mega Drive realization.
    std::vector<creative_attribution_evidence> composition_evidence{
        support(
            creative_attribution_evidence_kind::composition_structure,
            creative_attribution_role::composer,
            evidence_status::hypothesis,
            0.91,
            "cross-game-symbolic-control",
            "melodic contour, phrase grammar and harmonic motion match held-out work"),
        support(
            creative_attribution_evidence_kind::version_lineage,
            creative_attribution_role::composer,
            evidence_status::derived,
            0.85,
            "prototype-final-work-identity",
            "musical skeleton persists through a changed realization"),
    };
    const auto composer = make_creative_attribution_hypothesis(
        "candidate-composer",
        creative_attribution_role::composer,
        0.88,
        composition_evidence);
    assert(composer.role_specific_support);
    assert(composer.cross_domain_grounded);
    assert(close_enough(composer.independent_support_ceiling, 0.85));
    assert(close_enough(composer.confidence, 0.85));

    const auto programmer_from_composition = make_creative_attribution_hypothesis(
        "candidate-composer",
        creative_attribution_role::arranger_programmer,
        0.88,
        composition_evidence);
    assert(!programmer_from_composition.role_specific_support);
    assert(close_enough(
        programmer_from_composition.confidence,
        creative_attribution_no_role_support_ceiling));

    // Control 3: inherited artist/credit metadata is evidence about the
    // metadata source, not strong historical attribution by itself.
    const auto metadata_only = make_creative_attribution_hypothesis(
        "inherited-label-candidate",
        creative_attribution_role::composer,
        0.99,
        {support(
            creative_attribution_evidence_kind::metadata_label,
            creative_attribution_role::composer,
            evidence_status::exact,
            1.0,
            "legacy-credit-table",
            "exactly records the inherited label but does not establish its interpretation")});
    assert(metadata_only.metadata_only);
    assert(!metadata_only.role_specific_support);
    assert(close_enough(
        metadata_only.confidence,
        creative_attribution_metadata_only_ceiling));

    // Control 4: newer role-specific evidence is allowed to contradict an old
    // interpretation. The old label is preserved, but it cannot freeze the
    // hypothesis when a strong conflict appears.
    const auto revisable = make_creative_attribution_hypothesis(
        "inherited-label-candidate",
        creative_attribution_role::arranger_programmer,
        0.93,
        {
            support(
                creative_attribution_evidence_kind::metadata_label,
                creative_attribution_role::arranger_programmer,
                evidence_status::exact,
                1.0,
                "legacy-credit-table",
                "older interpretation associates this cue with the candidate"),
            support(
                creative_attribution_evidence_kind::arrangement_execution,
                creative_attribution_role::arranger_programmer,
                evidence_status::derived,
                0.86,
                "vgm-realization-control",
                "some realization features remain compatible with the inherited candidate"),
            counter(
                creative_attribution_role::arranger_programmer,
                0.92,
                "new-source-layout-and-control-evidence",
                "newer source-layout and technical evidence conflicts with the inherited interpretation"),
        });
    assert(revisable.strong_conflict_present);
    assert(close_enough(
        revisable.confidence,
        creative_attribution_strong_conflict_ceiling));

    // Control 5: recollection is useful but not exact documentary grounding.
    // A remembered conversation whose original messages are lost must not act
    // like a preserved role-specific primary document, and a 0.70 recollection
    // cannot be inflated to the generic single-domain ceiling.
    const auto recollection = make_creative_attribution_hypothesis(
        "recollected-candidate",
        creative_attribution_role::composer,
        0.90,
        {support(
            creative_attribution_evidence_kind::external_recollection,
            creative_attribution_role::composer,
            evidence_status::hypothesis,
            0.70,
            "researcher-recollection",
            "first-hand contact was reported but the original messages are no longer recoverable")});
    assert(recollection.role_specific_support);
    assert(!recollection.documentary_grounded);
    assert(!recollection.cross_domain_grounded);
    assert(close_enough(recollection.independent_support_ceiling, 0.70));
    assert(close_enough(recollection.confidence, 0.70));

    // Control 6: exact role-specific documentary evidence is a different
    // epistemic class and may remain strong even when lower-level evidence is
    // awkward or conflicting. It is still bounded by the documentary evidence
    // itself and the proposed confidence.
    const auto documented = make_creative_attribution_hypothesis(
        "documented-candidate",
        creative_attribution_role::arranger_programmer,
        0.97,
        {
            support(
                creative_attribution_evidence_kind::documentary_role_credit,
                creative_attribution_role::arranger_programmer,
                evidence_status::exact,
                0.99,
                "preserved-primary-credit",
                "preserved source explicitly credits this candidate for arrangement/programming"),
            counter(
                creative_attribution_role::arranger_programmer,
                0.88,
                "technical-outlier",
                "this realization is an outlier relative to the candidate's other controls"),
        });
    assert(documented.documentary_grounded);
    assert(documented.strong_conflict_present);
    assert(close_enough(documented.independent_support_ceiling, 0.99));
    assert(close_enough(documented.confidence, 0.97));

    // Control 7: prototype and final can keep one composition identity while
    // supporting different realization authors. These hypotheses are allowed
    // to coexist rather than being averaged into one artist score.
    const auto same_work_composer = make_creative_attribution_hypothesis(
        "composer-A",
        creative_attribution_role::composer,
        0.86,
        {
            support(
                creative_attribution_evidence_kind::composition_structure,
                creative_attribution_role::composer,
                evidence_status::derived,
                0.91,
                "prototype-final-symbolic-alignment",
                "core melody, bass motion and phrase plan survive across versions"),
            support(
                creative_attribution_evidence_kind::version_lineage,
                creative_attribution_role::composer,
                evidence_status::derived,
                0.95,
                "work-version-control",
                "prototype and final are independently established as versions of the same cue"),
        });
    const auto prototype_programmer = make_creative_attribution_hypothesis(
        "programmer-P",
        creative_attribution_role::arranger_programmer,
        0.84,
        {
            support(
                creative_attribution_evidence_kind::arrangement_execution,
                creative_attribution_role::arranger_programmer,
                evidence_status::derived,
                0.90,
                "prototype-realization-controls",
                "prototype control vocabulary matches programmer P"),
            support(
                creative_attribution_evidence_kind::version_lineage,
                creative_attribution_role::arranger_programmer,
                evidence_status::derived,
                0.82,
                "prototype-realization-lineage",
                "realization lineage is internally consistent within the prototype family"),
        });
    const auto final_programmer = make_creative_attribution_hypothesis(
        "programmer-F",
        creative_attribution_role::arranger_programmer,
        0.87,
        {
            support(
                creative_attribution_evidence_kind::arrangement_execution,
                creative_attribution_role::arranger_programmer,
                evidence_status::derived,
                0.93,
                "final-realization-controls",
                "final control vocabulary matches programmer F"),
            support(
                creative_attribution_evidence_kind::version_lineage,
                creative_attribution_role::arranger_programmer,
                evidence_status::derived,
                0.84,
                "final-realization-lineage",
                "final realization diverges from prototype while retaining the musical work"),
        });

    assert(close_enough(same_work_composer.confidence, 0.86));
    assert(close_enough(prototype_programmer.confidence, 0.82));
    assert(close_enough(final_programmer.confidence, 0.84));
    assert(prototype_programmer.candidate != final_programmer.candidate);

    // Control 8: multiple weak role-specific hints still cannot become strong
    // merely by occupying different evidence domains.
    const auto weak_multidomain = make_creative_attribution_hypothesis(
        "weak-candidate",
        creative_attribution_role::composer,
        0.95,
        {
            support(
                creative_attribution_evidence_kind::composition_structure,
                creative_attribution_role::composer,
                evidence_status::hypothesis,
                0.55,
                "weak-structure",
                "weak structural resemblance"),
            support(
                creative_attribution_evidence_kind::version_lineage,
                creative_attribution_role::composer,
                evidence_status::hypothesis,
                0.58,
                "weak-lineage",
                "weak project/version compatibility"),
        });
    assert(weak_multidomain.role_support_observations == 2);
    assert(weak_multidomain.grounding_support_observations == 0);
    assert(!weak_multidomain.cross_domain_grounded);
    assert(close_enough(
        weak_multidomain.confidence,
        creative_attribution_weak_support_ceiling));

    return 0;
}
