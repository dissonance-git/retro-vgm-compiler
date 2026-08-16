#include "model/composer_grammar_evidence.h"

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

composer_grammar_observation support(
    const char* soundtrack,
    const char* work,
    composer_representation_kind representation,
    composer_grammar_dimension dimension,
    creative_attribution_role role,
    double confidence,
    const char* source,
    const char* detail) {
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
        detail,
    };
}

composer_grammar_observation counter(
    const char* soundtrack,
    const char* work,
    composer_representation_kind representation,
    composer_grammar_dimension dimension,
    creative_attribution_role role,
    double confidence,
    const char* source,
    const char* detail) {
    return {
        soundtrack,
        work,
        representation,
        dimension,
        role,
        composer_grammar_polarity::counters,
        evidence_status::derived,
        confidence,
        source,
        detail,
    };
}

composer_grammar_intervention intervention(
    composer_confound_kind confound,
    bool survived,
    double confidence,
    const char* source,
    const char* detail) {
    return {confound, survived, confidence, source, detail};
}

} // namespace

int main() {
    // Control 1: many representations of one work improve our understanding
    // of that work, but do not establish a recurring composer habit.
    const auto one_work_many_views = make_composer_grammar_rule(
        "composer-A",
        creative_attribution_role::composer,
        "delayed-return-rule",
        0.95,
        {
            support(
                "soundtrack-1",
                "work-1",
                composer_representation_kind::symbolic_sequence,
                composer_grammar_dimension::phrase_form,
                creative_attribution_role::composer,
                0.95,
                "native-sequence",
                "phrase extension is explicit in the symbolic sequence"),
            support(
                "soundtrack-1",
                "work-1",
                composer_representation_kind::synthesis_runtime,
                composer_grammar_dimension::performance_execution,
                creative_attribution_role::composer,
                0.90,
                "vgm-runtime",
                "runtime articulation preserves the same delayed arrival"),
            support(
                "soundtrack-1",
                "work-1",
                composer_representation_kind::rendered_audio,
                composer_grammar_dimension::arrangement_orchestration,
                creative_attribution_role::composer,
                0.88,
                "audio-analysis",
                "auditory grouping confirms the widened return"),
        });

    assert(one_work_many_views.cross_representation_grounded);
    assert(one_work_many_views.multi_dimension_grounded);
    assert(!one_work_many_views.cross_work_grounded);
    assert(!one_work_many_views.cross_soundtrack_grounded);
    assert(close_enough(one_work_many_views.confidence, composer_grammar_single_work_ceiling));

    // Control 2: several independent works inside one soundtrack can establish
    // a soundtrack-local grammar, but still cannot earn the strongest general
    // composer confidence because shared production conditions remain.
    const auto one_soundtrack_many_works = make_composer_grammar_rule(
        "composer-A",
        creative_attribution_role::composer,
        "bass-under-retained-melody",
        0.94,
        {
            support(
                "soundtrack-1",
                "work-1",
                composer_representation_kind::symbolic_sequence,
                composer_grammar_dimension::bass_harmony,
                creative_attribution_role::composer,
                0.91,
                "sequence-analysis-1",
                "bass changes under retained upper material"),
            support(
                "soundtrack-1",
                "work-2",
                composer_representation_kind::symbolic_sequence,
                composer_grammar_dimension::bass_harmony,
                creative_attribution_role::composer,
                0.89,
                "sequence-analysis-2",
                "independent cue uses the same relational strategy"),
            support(
                "soundtrack-1",
                "work-3",
                composer_representation_kind::rendered_audio,
                composer_grammar_dimension::phrase_form,
                creative_attribution_role::composer,
                0.86,
                "audio-analysis-3",
                "strategy reappears at a phrase extension in another cue"),
        });

    assert(one_soundtrack_many_works.cross_work_grounded);
    assert(!one_soundtrack_many_works.cross_soundtrack_grounded);
    assert(close_enough(
        one_soundtrack_many_works.confidence,
        composer_grammar_single_soundtrack_ceiling));

    // Control 3: the same relational habit recurring in independent works on
    // different soundtracks, through different representations, can retain a
    // strong confidence proposal.
    const auto cross_soundtrack_rule = make_composer_grammar_rule(
        "composer-A",
        creative_attribution_role::composer,
        "fragment-sequence-reharmonized-return",
        0.92,
        {
            support(
                "soundtrack-genesis",
                "work-zone-A",
                composer_representation_kind::symbolic_sequence,
                composer_grammar_dimension::motif_development,
                creative_attribution_role::composer,
                0.93,
                "smps-analysis",
                "motif fragments, sequences, then returns over changed bass"),
            support(
                "soundtrack-snes",
                "work-stage-B",
                composer_representation_kind::synthesis_runtime,
                composer_grammar_dimension::bass_harmony,
                creative_attribution_role::composer,
                0.89,
                "spc-analysis",
                "persistent parts recover the same bass-against-retained-motif relation"),
            support(
                "soundtrack-playstation",
                "work-scene-C",
                composer_representation_kind::external_transcription,
                composer_grammar_dimension::phrase_form,
                creative_attribution_role::composer,
                0.88,
                "score-transcription-analysis",
                "independent soundtrack exhibits the same transformation at formal return"),
        },
        {
            intervention(
                composer_confound_kind::patch_sample_identity,
                true,
                0.92,
                "ablation-1",
                "rule remains after patch/sample identity is removed"),
            intervention(
                composer_confound_kind::platform,
                true,
                0.91,
                "ablation-2",
                "rule remains in leave-one-platform-out evaluation"),
            intervention(
                composer_confound_kind::soundtrack_local_context,
                true,
                0.90,
                "ablation-3",
                "rule remains in leave-one-soundtrack-out evaluation"),
        });

    assert(cross_soundtrack_rule.cross_work_grounded);
    assert(cross_soundtrack_rule.cross_soundtrack_grounded);
    assert(cross_soundtrack_rule.cross_representation_grounded);
    assert(cross_soundtrack_rule.multi_dimension_grounded);
    assert(cross_soundtrack_rule.intervention_grounded);
    assert(close_enough(cross_soundtrack_rule.confidence, 0.92));

    // Control 4: evidence from several soundtracks still cannot be promoted to
    // composer evidence if its historical role is actually arrangement/programming.
    const auto wrong_role = make_composer_grammar_rule(
        "programmer-B",
        creative_attribution_role::composer,
        "modulation-and-channel-layout",
        0.96,
        {
            support(
                "soundtrack-1",
                "work-1",
                composer_representation_kind::driver_execution,
                composer_grammar_dimension::performance_execution,
                creative_attribution_role::arranger_programmer,
                0.96,
                "driver-control-1",
                "modulation macro and channel layout recur"),
            support(
                "soundtrack-2",
                "work-2",
                composer_representation_kind::driver_execution,
                composer_grammar_dimension::arrangement_orchestration,
                creative_attribution_role::arranger_programmer,
                0.94,
                "driver-control-2",
                "same implementation grammar appears in another soundtrack"),
        });

    assert(wrong_role.supporting_observations == 0);
    assert(close_enough(wrong_role.confidence, composer_grammar_no_role_support_ceiling));

    // Control 5: a rule that fails a strong confound intervention must be
    // demoted even when it looked excellent across works and soundtracks.
    const auto patch_shortcut = make_composer_grammar_rule(
        "composer-C",
        creative_attribution_role::composer,
        "bright-return-signature",
        0.95,
        {
            support(
                "soundtrack-1",
                "work-1",
                composer_representation_kind::synthesis_runtime,
                composer_grammar_dimension::timbre_synthesis,
                creative_attribution_role::composer,
                0.94,
                "synthesis-control-1",
                "bright patch family appears at returns"),
            support(
                "soundtrack-2",
                "work-2",
                composer_representation_kind::rendered_audio,
                composer_grammar_dimension::arrangement_orchestration,
                creative_attribution_role::composer,
                0.90,
                "audio-control-2",
                "auditory brightness increases at returns"),
        },
        {
            intervention(
                composer_confound_kind::patch_sample_identity,
                false,
                0.95,
                "patch-mask",
                "candidate signal disappears when shared patch identity is removed"),
        });

    assert(patch_shortcut.strong_confound_failure);
    assert(close_enough(patch_shortcut.confidence, composer_grammar_failed_confound_ceiling));

    // Control 6: strong contradictory evidence in another soundtrack keeps a
    // proposed universal rule below the strong threshold instead of forcing a
    // frozen fingerprint onto composer evolution.
    const auto evolving_composer = make_composer_grammar_rule(
        "composer-D",
        creative_attribution_role::composer,
        "always-closes-loop-with-full-cadence",
        0.90,
        {
            support(
                "early-soundtrack",
                "work-1",
                composer_representation_kind::symbolic_sequence,
                composer_grammar_dimension::phrase_form,
                creative_attribution_role::composer,
                0.90,
                "early-control",
                "full cadential closure occurs at the loop"),
            support(
                "middle-soundtrack",
                "work-2",
                composer_representation_kind::symbolic_sequence,
                composer_grammar_dimension::phrase_form,
                creative_attribution_role::composer,
                0.87,
                "middle-control",
                "similar full closure occurs in another soundtrack"),
            counter(
                "late-soundtrack",
                "work-3",
                composer_representation_kind::symbolic_sequence,
                composer_grammar_dimension::phrase_form,
                creative_attribution_role::composer,
                0.94,
                "late-control",
                "later soundtrack systematically avoids full closure at loops"),
        });

    assert(evolving_composer.strong_conflict_present);
    assert(close_enough(evolving_composer.confidence, composer_grammar_strong_conflict_ceiling));

    // Control 7: a validated cross-soundtrack grammar can enter the outer
    // attribution layer as its own evidence domain without pretending it was
    // merely a score statistic.
    const auto grammar_evidence = as_creator_grammar_evidence(cross_soundtrack_rule);
    assert(grammar_evidence.kind == creative_attribution_evidence_kind::creator_grammar);
    assert(grammar_evidence.role_scope == creative_attribution_role::composer);
    assert(close_enough(grammar_evidence.confidence, 0.92));

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
                "held-out cue belongs to the relevant project and candidate set without supplying track authorship",
            },
        });

    assert(attribution.cross_domain_grounded);
    assert(close_enough(attribution.confidence, 0.90));

    return 0;
}
