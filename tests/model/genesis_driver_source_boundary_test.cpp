#include "model/analysis_feature.h"

#include <cassert>
#include <string>

using namespace vgmtooling::model;

namespace {

const std::string& string_value(const analysis_feature* feature) {
    assert(feature != nullptr);
    assert(feature->value.has_value());
    return std::get<std::string>(*feature->value);
}

} // namespace

int main() {
    // VGM preserves a sample-timed device-facing command stream. Those facts
    // can be exact with respect to the VGM artifact without implying that the
    // original driver's logical program survived in the file.
    analysis_feature_set vgm_only;

    auto chip_target = present_feature(
        "genesis_device_family",
        semantic_layer::synthesis,
        attribute_value{std::string{"YM2612+SN76489"}},
        evidence_status::exact,
        1.0);
    chip_target.provenance.push_back({
        evidence_status::exact,
        1.0,
        "VGMRips VGM Specification",
        std::nullopt,
        "VGM command stream contains timed writes to the Genesis sound devices",
    });
    vgm_only.add(std::move(chip_target));

    auto sample_timebase = present_feature(
        "vgm_timebase_hz",
        semantic_layer::source_representation,
        attribute_value{std::uint64_t{44100}},
        evidence_status::exact,
        1.0,
        "Hz");
    sample_timebase.provenance.push_back({
        evidence_status::exact,
        1.0,
        "VGMRips VGM Specification",
        std::nullopt,
        "VGM wait/sample values use the 44,100-sample-per-second timebase",
    });
    vgm_only.add(std::move(sample_timebase));

    vgm_only.add(unresolved_feature(
        "original_driver_family",
        semantic_layer::driver_execution,
        feature_availability::unavailable,
        "a device-facing VGM trace does not intrinsically encode the original driver family",
        "vgm-source-boundary"));
    vgm_only.add(unresolved_feature(
        "original_logical_track_identity",
        semantic_layer::driver_execution,
        feature_availability::unavailable,
        "physical chip activity does not intrinsically preserve the original logical track identity",
        "vgm-source-boundary"));
    vgm_only.add(unresolved_feature(
        "source_command_grammar",
        semantic_layer::authored_program,
        feature_availability::unavailable,
        "the original sequence command grammar is outside the normal VGM device-log representation",
        "vgm-source-boundary"));

    // Known SMPS and GEMS source controls deliberately target the same broad
    // Genesis hardware while retaining different driver identities and source
    // organizations. Equal hardware therefore cannot be used as driver proof.
    analysis_feature_set smps_source;
    smps_source.add(present_feature(
        "genesis_device_family",
        semantic_layer::synthesis,
        attribute_value{std::string{"YM2612+SN76489"}},
        evidence_status::exact,
        1.0));
    smps_source.add(present_feature(
        "driver_family",
        semantic_layer::driver_execution,
        attribute_value{std::string{"SMPS Z80 Type 2"}},
        evidence_status::exact,
        1.0));
    smps_source.add(present_feature(
        "source_command_grammar",
        semantic_layer::authored_program,
        attribute_value{std::string{"SMPS coordination flags"}},
        evidence_status::exact,
        1.0));
    smps_source.add(present_feature(
        "source_data_layout",
        semantic_layer::source_representation,
        attribute_value{std::string{"song+instrument+modulation+PSG-envelope+driver-config"}},
        evidence_status::exact,
        1.0));
    smps_source.add(unresolved_feature(
        "composer_attribution",
        semantic_layer::musicological_context,
        feature_availability::unknown,
        "known SMPS driver/source identity does not establish composition authorship",
        "driver-role-boundary"));

    analysis_feature_set gems_source;
    gems_source.add(present_feature(
        "genesis_device_family",
        semantic_layer::synthesis,
        attribute_value{std::string{"YM2612+SN76489"}},
        evidence_status::exact,
        1.0));
    gems_source.add(present_feature(
        "driver_family",
        semantic_layer::driver_execution,
        attribute_value{std::string{"GEMS 2.8"}},
        evidence_status::exact,
        1.0));
    gems_source.add(present_feature(
        "source_data_layout",
        semantic_layer::source_representation,
        attribute_value{std::string{"instrument+envelope+sequence+sample"}},
        evidence_status::exact,
        1.0));
    gems_source.add(unresolved_feature(
        "composer_attribution",
        semantic_layer::musicological_context,
        feature_availability::unknown,
        "known GEMS driver/source identity does not establish composition authorship",
        "driver-role-boundary"));

    assert(vgm_only.find("original_driver_family")->availability ==
           feature_availability::unavailable);
    assert(vgm_only.find("original_logical_track_identity")->availability ==
           feature_availability::unavailable);
    assert(vgm_only.find("source_command_grammar")->availability ==
           feature_availability::unavailable);

    assert(string_value(smps_source.find("genesis_device_family")) ==
           string_value(gems_source.find("genesis_device_family")));
    assert(string_value(smps_source.find("driver_family")) !=
           string_value(gems_source.find("driver_family")));
    assert(string_value(smps_source.find("source_data_layout")) !=
           string_value(gems_source.find("source_data_layout")));

    assert(smps_source.find("composer_attribution")->availability ==
           feature_availability::unknown);
    assert(gems_source.find("composer_attribution")->availability ==
           feature_availability::unknown);

    return 0;
}
