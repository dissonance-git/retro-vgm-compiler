#include "model/analysis_feature.h"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>

using namespace vgmtooling::model;

namespace {

template <typename Fn>
bool throws_invalid_argument(Fn&& fn) {
    try {
        fn();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

} // namespace

int main() {
    analysis_feature_set genesis;

    auto genesis_channel = present_feature(
        "physical_channel",
        attribute_value{std::uint64_t{2}},
        evidence_status::derived,
        1.0,
        "channel");
    genesis_channel.provenance.push_back({
        evidence_status::derived,
        1.0,
        "genesis-performance-fixture",
        std::nullopt,
        "physical channel recovered from decoded YM2612 execution",
    });
    genesis.add(std::move(genesis_channel));

    auto genesis_pitch = present_feature(
        "device_native_pitch_code",
        attribute_value{std::uint64_t{0x2A4}},
        evidence_status::derived,
        1.0,
        "device_native");
    genesis_pitch.provenance.push_back({
        evidence_status::derived,
        1.0,
        "genesis-pitch-fixture",
        std::nullopt,
        "YM2612 F-number/block state is available while authored note identity is not",
    });
    genesis.add(std::move(genesis_pitch));

    genesis.add(unresolved_feature(
        "normalized_absolute_pitch",
        feature_availability::unknown,
        "meaningful musical quantity, but this device-native fixture has not established an authored/normalized pitch",
        "genesis-pitch-fixture"));
    genesis.add(unresolved_feature(
        "original_driver_track",
        feature_availability::unavailable,
        "the VGM-style execution observation does not expose the original logical driver track",
        "genesis-trace-fixture"));
    genesis.add(unresolved_feature(
        "sample_identity",
        feature_availability::not_applicable,
        "ordinary PSG tone generation does not reference a stored sample object",
        "genesis-psg-fixture"));

    const analysis_feature* genesis_unknown = genesis.find("normalized_absolute_pitch");
    const analysis_feature* genesis_unavailable = genesis.find("original_driver_track");
    const analysis_feature* genesis_not_applicable = genesis.find("sample_identity");
    assert(genesis_unknown != nullptr);
    assert(genesis_unavailable != nullptr);
    assert(genesis_not_applicable != nullptr);
    assert(genesis_unknown->availability == feature_availability::unknown);
    assert(genesis_unavailable->availability == feature_availability::unavailable);
    assert(genesis_not_applicable->availability == feature_availability::not_applicable);
    assert(genesis_unknown->availability != genesis_unavailable->availability);
    assert(genesis_unavailable->availability != genesis_not_applicable->availability);
    assert(!genesis_unknown->value.has_value());
    assert(!genesis_unavailable->value.has_value());
    assert(!genesis_not_applicable->value.has_value());

    analysis_feature_set spc;

    auto spc_voice = present_feature(
        "physical_voice",
        attribute_value{std::uint64_t{5}},
        evidence_status::exact,
        1.0,
        "slot");
    spc_voice.provenance.push_back({
        evidence_status::exact,
        1.0,
        "spc-runtime-fixture",
        std::nullopt,
        "instrumented S-DSP runtime exposes the physical voice index directly",
        to_flags(provenance_flag::runtime_capture),
    });
    spc.add(std::move(spc_voice));

    auto spc_pitch_rate = present_feature(
        "device_native_pitch_rate",
        attribute_value{std::uint64_t{0x1000}},
        evidence_status::exact,
        1.0,
        "device_native");
    spc_pitch_rate.provenance.push_back({
        evidence_status::exact,
        1.0,
        "spc-runtime-fixture",
        std::nullopt,
        "instrumented S-DSP runtime exposes pitch rate without proving sample root tuning",
        to_flags(provenance_flag::runtime_capture),
    });
    spc.add(std::move(spc_pitch_rate));

    auto spc_source_index = present_feature(
        "source_index",
        attribute_value{std::uint64_t{7}},
        evidence_status::exact,
        1.0,
        "slot");
    spc_source_index.provenance.push_back({
        evidence_status::exact,
        1.0,
        "spc-runtime-fixture",
        std::nullopt,
        "runtime source index observed at the DSP boundary",
        to_flags(provenance_flag::runtime_capture),
    });
    spc.add(std::move(spc_source_index));

    spc.add(unresolved_feature(
        "sample_root_tuning",
        feature_availability::unknown,
        "the sample is known but its authored root tuning is not established",
        "spc-runtime-fixture"));
    spc.add(unresolved_feature(
        "normalized_absolute_pitch",
        feature_availability::unknown,
        "pitch rate alone cannot establish absolute musical pitch without source/tuning continuity",
        "spc-runtime-fixture"));
    spc.add(unresolved_feature(
        "original_driver_track",
        feature_availability::unavailable,
        "the current S-DSP runtime boundary does not expose validated driver-track identity",
        "spc-runtime-fixture"));

    assert(spc.find("device_native_pitch_rate")->availability == feature_availability::present);
    assert(spc.find("sample_root_tuning")->availability == feature_availability::unknown);
    assert(spc.find("original_driver_track")->availability == feature_availability::unavailable);

    // Presentness and evidential strength are independent axes.
    assert(genesis.find("device_native_pitch_code")->status == evidence_status::derived);
    assert(spc.find("device_native_pitch_rate")->status == evidence_status::exact);

    // Missing support must not be silently encoded as a value.
    analysis_feature invalid_unknown;
    invalid_unknown.name = "bad_unknown";
    invalid_unknown.availability = feature_availability::unknown;
    invalid_unknown.value = attribute_value{std::uint64_t{0}};
    assert(throws_invalid_argument([&] { validate_analysis_feature(invalid_unknown); }));

    analysis_feature missing_value;
    missing_value.name = "missing_value";
    missing_value.availability = feature_availability::present;
    missing_value.status = evidence_status::derived;
    missing_value.confidence = 1.0;
    assert(throws_invalid_argument([&] { validate_analysis_feature(missing_value); }));

    analysis_feature missing_status;
    missing_status.name = "missing_status";
    missing_status.availability = feature_availability::present;
    missing_status.value = attribute_value{std::uint64_t{1}};
    missing_status.confidence = 1.0;
    assert(throws_invalid_argument([&] { validate_analysis_feature(missing_status); }));

    analysis_feature missing_confidence;
    missing_confidence.name = "missing_confidence";
    missing_confidence.availability = feature_availability::present;
    missing_confidence.value = attribute_value{std::uint64_t{1}};
    missing_confidence.status = evidence_status::derived;
    assert(throws_invalid_argument([&] { validate_analysis_feature(missing_confidence); }));

    assert(throws_invalid_argument([] {
        (void)present_feature(
            "bad_confidence",
            attribute_value{std::uint64_t{1}},
            evidence_status::hypothesis,
            1.1);
    }));

    assert(throws_invalid_argument([&] {
        genesis.add(present_feature(
            "physical_channel",
            attribute_value{std::uint64_t{3}},
            evidence_status::derived,
            1.0,
            "channel"));
    }));

    assert(throws_invalid_argument([] {
        (void)unresolved_feature("invalid", feature_availability::present);
    }));

    return 0;
}
