#include "model/analysis_feature.h"

#include <cassert>
#include <string>

using namespace vgmtooling::model;

int main() {
    // Exact low-level execution identity can be known without proving who
    // composed the music carried by that implementation.
    analysis_feature_set execution;
    execution.add(present_feature(
        "driver_family_id",
        semantic_layer::driver_execution,
        attribute_value{std::string{"driver-family-A"}},
        evidence_status::exact,
        1.0));
    execution.add(present_feature(
        "modulation_macro_signature",
        semantic_layer::driver_execution,
        attribute_value{std::string{"macro-signature-17"}},
        evidence_status::derived,
        0.99));

    assert(execution.find("driver_family_id")->status == evidence_status::exact);
    assert(execution.find("driver_family_id")->claim_layer == semantic_layer::driver_execution);

    // Metadata values are exact only relative to the source that supplied
    // them. A stale embedded artist tag and a curated external-library artist
    // tag may disagree without either value becoming composer proof.
    analysis_feature_set metadata;
    auto embedded_artist = present_feature(
        "embedded_artist_tag",
        semantic_layer::source_representation,
        attribute_value{std::string{"legacy-artist-string"}},
        evidence_status::exact,
        1.0);
    embedded_artist.provenance.push_back({
        evidence_status::exact,
        1.0,
        "embedded-game-music-metadata",
        std::nullopt,
        "exact as artifact metadata only; not authoritative attribution evidence",
    });
    metadata.add(std::move(embedded_artist));

    auto external_artist = present_feature(
        "external_library_artist_tag",
        semantic_layer::musicological_context,
        attribute_value{std::string{"curated-artist-string"}},
        evidence_status::exact,
        1.0);
    external_artist.provenance.push_back({
        evidence_status::exact,
        1.0,
        "helix-foobar-external-tags",
        std::nullopt,
        "curated user-facing library metadata; exact relative to the external tag source, not composer proof",
    });
    metadata.add(std::move(external_artist));

    metadata.add(present_feature(
        "sound_team_metadata",
        semantic_layer::musicological_context,
        attribute_value{std::string{"team-A"}},
        evidence_status::exact,
        1.0));
    metadata.add(unresolved_feature(
        "metadata_only_composer_attribution",
        semantic_layer::musicological_context,
        feature_availability::unknown,
        "embedded artist, external library artist, and sound-team metadata do not establish track-level composition authorship",
        "metadata-attribution-boundary"));

    assert(std::get<std::string>(metadata.find("embedded_artist_tag")->value.value()) !=
           std::get<std::string>(metadata.find("external_library_artist_tag")->value.value()));
    assert(metadata.find("embedded_artist_tag")->claim_layer == semantic_layer::source_representation);
    assert(metadata.find("external_library_artist_tag")->claim_layer == semantic_layer::musicological_context);
    assert(metadata.find("metadata_only_composer_attribution")->availability == feature_availability::unknown);

    // A technical fingerprint may support a sound-programming attribution
    // hypothesis. Its role scope is explicit in the feature identity and its
    // provenance. It does not become a composer attribution.
    analysis_feature_set attribution;
    auto programmer = present_feature(
        "sound_programmer_attribution_candidate",
        semantic_layer::musicological_context,
        attribute_value{std::string{"programmer-A"}},
        evidence_status::hypothesis,
        0.94);
    programmer.provenance.push_back({
        evidence_status::hypothesis,
        0.94,
        "driver-fingerprint-control",
        std::nullopt,
        "match uses driver grammar and modulation idioms; scope is sound programming, not composition",
    });
    attribution.add(std::move(programmer));

    attribution.add(unresolved_feature(
        "composer_attribution",
        semantic_layer::musicological_context,
        feature_availability::unknown,
        "driver and sound-programming evidence does not establish who wrote the composition",
        "role-separation-control"));

    assert(attribution.find("sound_programmer_attribution_candidate")->status == evidence_status::hypothesis);
    assert(attribution.find("composer_attribution")->availability == feature_availability::unknown);

    // Independent musical-style evidence may point to a different person. The
    // coexistence is expected: composition and sound programming are different
    // historical contributions even when one person sometimes fills both roles.
    auto composer_style = present_feature(
        "composition_style_attribution_candidate",
        semantic_layer::musicological_context,
        attribute_value{std::string{"composer-B"}},
        evidence_status::hypothesis,
        0.72);
    composer_style.provenance.push_back({
        evidence_status::hypothesis,
        0.72,
        "held-out-composition-style-control",
        std::nullopt,
        "match uses melodic/rhythmic/formal features and excludes driver-family features",
    });
    attribution.add(std::move(composer_style));

    assert(std::get<std::string>(
               attribution.find("sound_programmer_attribution_candidate")->value.value()) == "programmer-A");
    assert(std::get<std::string>(
               attribution.find("composition_style_attribution_candidate")->value.value()) == "composer-B");
    assert(attribution.find("composer_attribution")->availability == feature_availability::unknown);

    // Programmed expression is also not implementation residue by definition.
    // Exact driver controls may support a higher musical-performance claim
    // while remaining distinct from the interpretation of what those controls
    // mean musically.
    analysis_feature_set expression;
    expression.add(present_feature(
        "programmed_pitch_envelope_id",
        semantic_layer::driver_execution,
        attribute_value{std::string{"pitch-envelope-4"}},
        evidence_status::exact,
        1.0));

    auto gesture = present_feature(
        "expressive_gesture",
        semantic_layer::musical_performance,
        attribute_value{std::string{"scooped_attack"}},
        evidence_status::derived,
        0.93);
    gesture.provenance.push_back({
        evidence_status::derived,
        0.93,
        "programmed-expression-control",
        std::nullopt,
        "gesture is derived from exact pitch-envelope execution; the interpretation does not replace the envelope",
    });
    expression.add(std::move(gesture));

    assert(expression.find("programmed_pitch_envelope_id")->status == evidence_status::exact);
    assert(expression.find("programmed_pitch_envelope_id")->claim_layer == semantic_layer::driver_execution);
    assert(expression.find("expressive_gesture")->claim_layer == semantic_layer::musical_performance);
    assert(expression.find("expressive_gesture")->status == evidence_status::derived);

    return 0;
}
