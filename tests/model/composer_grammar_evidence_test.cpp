#include "model/composer_grammar_evidence.h"

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

bool close_enough(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1e-9;
}

composer_grammar_observation support(
    const char* soundtrack,
    const char* work,
    composer_representation_kind representation,
    composer_grammar_dimension dimension,
    creative_attribution_role role,
    double confidence,
    const char* source) {
    return {
        soundtrack,
        work,
        representation,
        dimension,
        role,
        composer_grammar_polarity::supports,
        evidence_status::derived,
        confidence,
        source,
        "synthetic composer-grammar support",
    };
}

composer_grammar_observation counter(
    const char* soundtrack,
    const char* work,
    double confidence,
    const char* source) {
    return {
        soundtrack,
        work,
        composer_representation_kind::symbolic_sequence,
        composer_grammar_dimension::phrase_form,
        creative_attribution_role::composer,
        composer_grammar_polarity::counters,
        evidence_status::derived,
        confidence,
        source,
        "synthetic composer-grammar counterevidence",
    };
}

} // namespace

int main() {
    // Many representations of one work improve work understanding, but remain
    // single-work evidence for composer grammar.
    const auto one_work = make_composer_grammar_rule(
        "composer-A",
        creative_attribution_role::composer,
        "one-work-many-views",
        0.95,
        {
            support("s1", "w1", composer_representation_kind::symbolic_sequence,
                    composer_grammar_dimension::phrase_form,
                    creative_attribution_role::composer, 0.95, "symbolic"),
            support("s1", "w1", composer_representation_kind::synthesis_runtime,
                    composer_grammar_dimension::performance_execution,
                    creative_attribution_role::composer, 0.90, "runtime"),
            support("s1", "w1", composer_representation_kind::rendered_audio,
                    composer_grammar_dimension::arrangement_orchestration,
                    creative_attribution_role::composer, 0.88, "audio"),
        });
    assert(one_work.cross_representation_grounded);
    assert(one_work.multi_dimension_grounded);
    assert(!one_work.cross_work_grounded);
    assert(!one_work.cross_soundtrack_grounded);
    assert(close_enough(one_work.confidence, composer_grammar_single_work_ceiling));

    // Independent works inside one soundtrack remain soundtrack-local.
    const auto one_soundtrack = make_composer_grammar_rule(
        "composer-A",
        creative_attribution_role::composer,
        "soundtrack-local-rule",
        0.94,
        {
            support("s1", "w1", composer_representation_kind::symbolic_sequence,
                    composer_grammar_dimension::bass_harmony,
                    creative_attribution_role::composer, 0.91, "w1"),
            support("s1", "w2", composer_representation_kind::symbolic_sequence,
                    composer_grammar_dimension::bass_harmony,
                    creative_attribution_role::composer, 0.89, "w2"),
            support("s1", "w3", composer_representation_kind::rendered_audio,
                    composer_grammar_dimension::phrase_form,
                    creative_attribution_role::composer, 0.86, "w3"),
        });
    assert(one_soundtrack.cross_work_grounded);
    assert(!one_soundtrack.cross_soundtrack_grounded);
    assert(close_enough(one_soundtrack.independent_support_ceiling, 0.89));
    assert(close_enough(one_soundtrack.confidence, composer_grammar_single_soundtrack_ceiling));

    // Strong recurrence across soundtracks is bounded by the weaker independent
    // soundtrack actually needed to establish the recurrence.
    const auto cross_soundtrack = make_composer_grammar_rule(
        "composer-A",
        creative_attribution_role::composer,
        "cross-soundtrack-rule",
        0.92,
        {
            support("genesis", "zone-A", composer_representation_kind::symbolic_sequence,
                    composer_grammar_dimension::motif_development,
                    creative_attribution_role::composer, 0.93, "smps"),
            support("snes", "stage-B", composer_representation_kind::synthesis_runtime,
                    composer_grammar_dimension::bass_harmony,
                    creative_attribution_role::composer, 0.89, "spc"),
            support("ps1", "scene-C", composer_representation_kind::external_transcription,
                    composer_grammar_dimension::phrase_form,
                    creative_attribution_role::composer, 0.88, "transcription"),
        },
        {
            {composer_confound_kind::patch_sample_identity, true, 0.92, "patch-mask", "survives"},
            {composer_confound_kind::platform, true, 0.91, "platform-holdout", "survives"},
        });
    assert(cross_soundtrack.cross_work_grounded);
    assert(cross_soundtrack.cross_soundtrack_grounded);
    assert(cross_soundtrack.cross_representation_grounded);
    assert(cross_soundtrack.intervention_grounded);
    assert(close_enough(cross_soundtrack.independent_support_ceiling, 0.89));
    assert(close_enough(cross_soundtrack.confidence, 0.89));

    // Wrong historical role cannot leak into composer grammar.
    const auto wrong_role = make_composer_grammar_rule(
        "programmer-B",
        creative_attribution_role::composer,
        "programming-fingerprint",
        0.96,
        {
            support("s1", "w1", composer_representation_kind::driver_execution,
                    composer_grammar_dimension::performance_execution,
                    creative_attribution_role::arranger_programmer, 0.96, "driver1"),
            support("s2", "w2", composer_representation_kind::driver_execution,
                    composer_grammar_dimension::arrangement_orchestration,
                    creative_attribution_role::arranger_programmer, 0.94, "driver2"),
        });
    assert(wrong_role.grounding_support_observations == 0);
    assert(close_enough(wrong_role.confidence, composer_grammar_no_role_support_ceiling));

    // Strong confound failure demotes an otherwise cross-soundtrack rule.
    const auto shortcut = make_composer_grammar_rule(
        "composer-C",
        creative_attribution_role::composer,
        "patch-shortcut",
        0.95,
        {
            support("s1", "w1", composer_representation_kind::synthesis_runtime,
                    composer_grammar_dimension::timbre_synthesis,
                    creative_attribution_role::composer, 0.94, "synth"),
            support("s2", "w2", composer_representation_kind::rendered_audio,
                    composer_grammar_dimension::arrangement_orchestration,
                    creative_attribution_role::composer, 0.90, "audio"),
        },
        {{composer_confound_kind::patch_sample_identity, false, 0.95, "mask", "signal disappears"}});
    assert(shortcut.strong_confound_failure);
    assert(close_enough(shortcut.confidence, composer_grammar_failed_confound_ceiling));

    // Career evolution/counterevidence is preserved rather than forcing a
    // universal static rule.
    const auto evolving = make_composer_grammar_rule(
        "composer-D",
        creative_attribution_role::composer,
        "always-full-cadence",
        0.90,
        {
            support("early", "w1", composer_representation_kind::symbolic_sequence,
                    composer_grammar_dimension::phrase_form,
                    creative_attribution_role::composer, 0.90, "early"),
            support("middle", "w2", composer_representation_kind::symbolic_sequence,
                    composer_grammar_dimension::phrase_form,
                    creative_attribution_role::composer, 0.87, "middle"),
            counter("late", "w3", 0.94, "late"),
        });
    assert(evolving.strong_conflict_present);
    assert(close_enough(evolving.confidence, composer_grammar_strong_conflict_ceiling));

    // The validated grammar can enter the outer role hypothesis, but the final
    // attribution is bounded again by its weaker independent outer evidence.
    const auto grammar_evidence = as_creator_grammar_evidence(cross_soundtrack);
    assert(close_enough(grammar_evidence.confidence, 0.89));
    const auto attribution = make_creative_attribution_hypothesis(
        "composer-A",
        creative_attribution_role::composer,
        0.90,
        {
            grammar_evidence,
            {
                creative_attribution_evidence_kind::version_lineage,
                creative_attribution_role::composer,
                creative_attribution_polarity::supports,
                evidence_status::derived,
                0.82,
                "historical-work-lineage",
                "candidate set and work lineage support",
            },
        });
    assert(attribution.cross_domain_grounded);
    assert(close_enough(attribution.independent_support_ceiling, 0.82));
    assert(close_enough(attribution.confidence, 0.82));

    // Two rhythm-only motif echoes at .55 do not become a composer rule merely
    // by carrying different soundtrack labels.
    const auto weak = make_composer_grammar_rule(
        "composer-E",
        creative_attribution_role::composer,
        "rhythm-only-echo",
        0.95,
        {
            support("s1", "w1", composer_representation_kind::synthesis_runtime,
                    composer_grammar_dimension::motif_development,
                    creative_attribution_role::composer, 0.55, "rhythm1"),
            support("s2", "w2", composer_representation_kind::synthesis_runtime,
                    composer_grammar_dimension::motif_development,
                    creative_attribution_role::composer, 0.55, "rhythm2"),
        });
    assert(weak.supporting_observations == 2);
    assert(weak.grounding_support_observations == 0);
    assert(!weak.cross_soundtrack_grounded);
    assert(close_enough(weak.confidence, composer_grammar_weak_support_ceiling));

    return 0;
}
