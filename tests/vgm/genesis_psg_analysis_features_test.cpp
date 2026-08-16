#include "components/vgm/enhancement/genesis_analysis_features.h"

#include <cmath>
#include <cstdint>
#include <string>

using namespace gameaudio::vgm;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second, double tolerance = 1e-9) {
    return std::fabs(first - second) <= tolerance;
}

} // namespace

int main() {
    musical_execution_graph graph;

    node episode;
    episode.kind = node_kind::voice_instance;
    episode.layer = semantic_layer::synthesis;
    episode.flow = flow_kind::stream;
    episode.label = "SN76489 tone episode";
    episode.active = time_span{at(100), at(220)};
    episode.attributes.push_back({
        "device_family", std::string{"SN76489"}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({
        "instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    episode.attributes.push_back({
        "physical_channel", std::uint64_t{1}, evidence_status::derived, 1.0, ""});
    const node_id episode_id = graph.add_node(std::move(episode));

    node onset;
    onset.kind = node_kind::musical_event;
    onset.layer = semantic_layer::musical_performance;
    onset.flow = flow_kind::event;
    onset.label = "SN76489 pitched activity onset";
    onset.active = time_span{at(100), std::nullopt};
    onset.attributes.push_back({
        "event_kind", std::string{"pitched_activity_onset"}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "device_family", std::string{"SN76489"}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "instance", std::uint64_t{0}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "physical_channel", std::uint64_t{1}, evidence_status::derived, 1.0, ""});
    onset.attributes.push_back({
        "device_pitch_code", std::uint64_t{0x125}, evidence_status::derived, 1.0, "device_native"});
    onset.attributes.push_back({
        "gate_or_level", std::uint64_t{2}, evidence_status::derived, 1.0, "device_native"});
    const node_id onset_id = graph.add_node(std::move(onset));

    edge realization;
    realization.kind = edge_kind::realizes;
    realization.from = onset_id;
    realization.to = episode_id;
    graph.add_edge(std::move(realization));

    genesis_pitch_clock_context clocks;
    clocks.ym2612_clock_hz = 7670454;
    clocks.sn76489_clock_hz = 3579545;
    clocks.source = "fixture-vgm-header";

    const auto features = extract_genesis_performance_analysis_features(
        graph,
        onset_id,
        &clocks);
    const auto* nominal = features.find("device_nominal_pitch_frequency_hz");
    const auto* performed = features.find("performed_pitch_frequency_hz");
    const auto* part = features.find("persistent_part_identity");

    CHECK(nominal != nullptr && performed != nullptr && part != nullptr);
    CHECK(nominal->availability == feature_availability::present);
    CHECK(performed->availability == feature_availability::present);
    CHECK(nominal->claim_layer == semantic_layer::synthesis);
    CHECK(performed->claim_layer == semantic_layer::musical_performance);
    CHECK(performed->status.has_value());
    CHECK(*performed->status == evidence_status::derived);
    CHECK(performed->confidence.has_value());
    CHECK(close_enough(*performed->confidence, 1.0));

    const auto* nominal_hz = std::get_if<double>(&*nominal->value);
    const auto* performed_hz = std::get_if<double>(&*performed->value);
    CHECK(nominal_hz != nullptr && performed_hz != nullptr);
    CHECK(close_enough(*nominal_hz, *performed_hz));
    CHECK(close_enough(*performed_hz, 381.7774104095563));

    // Knowing the square-wave fundamental does not manufacture a musical part
    // or tell us whether this source is melody, doubling, accompaniment, etc.
    CHECK(part->availability == feature_availability::unknown);

    // Without source-relative clock provenance, both frequency projections stay
    // unresolved even though the raw tone period remains available.
    const auto no_clock = extract_genesis_performance_analysis_features(
        graph,
        onset_id,
        nullptr);
    const auto* no_clock_performed = no_clock.find("performed_pitch_frequency_hz");
    CHECK(no_clock_performed != nullptr);
    CHECK(no_clock_performed->availability == feature_availability::unknown);

    return 0;
}
