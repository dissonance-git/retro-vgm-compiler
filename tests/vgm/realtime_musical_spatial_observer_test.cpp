#include "../../model/realtime_musical_spatial_observer.h"
#include "../../model/realtime_spatial_mix_budget.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

using observer_type = vgmtooling::model::realtime_musical_spatial_observer<4, 8>;

vgmtooling::model::spatial_audio_lane_view make_lane(
    const float* pcm,
    std::uint64_t source_id,
    std::uint64_t generation,
    vgmtooling::model::spatial_audio_lane_kind kind =
        vgmtooling::model::spatial_audio_lane_kind::dry_source,
    const std::uint8_t* availability = nullptr)
{
    vgmtooling::model::spatial_audio_lane_view lane{};
    lane.mono_pcm = pcm;
    lane.evidence.source_id = source_id;
    lane.evidence.generation = generation;
    lane.kind = kind;
    lane.availability = availability;
    return lane;
}

float profile_distance(
    const std::array<float, 3>& left,
    const std::array<float, 3>& right)
{
    float distance = 0.0f;
    for (std::size_t band = 0; band < left.size(); ++band)
        distance += std::fabs(left[band] - right[band]);
    return distance;
}

} // namespace

int main()
{
    constexpr double sample_rate = 48000.0;
    constexpr std::size_t frame_count = 4800;
    constexpr double pi = 3.141592653589793238462643383279502884;

    std::array<float, frame_count> low{};
    std::array<float, frame_count> high{};
    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        const double time = static_cast<double>(frame) / sample_rate;
        low[frame] = 0.5f * static_cast<float>(std::sin(2.0 * pi * 100.0 * time));
        high[frame] = 0.5f * static_cast<float>(std::sin(2.0 * pi * 8000.0 * time));
    }

    observer_type observer{};
    std::array<vgmtooling::model::spatial_audio_lane_view, 2> lanes{
        make_lane(low.data(), 1, 1),
        make_lane(high.data(), 2, 1),
    };
    const vgmtooling::model::spatial_source_block_view two_lane_block{
        lanes.data(),
        lanes.size(),
        frame_count,
    };

    // The causal one-pole statistic must discriminate low-frequency body from
    // high-frequency energy without claiming either source is musically bass.
    assert(observer.process(two_lane_block, sample_rate));
    assert(observer.source(0).audio_observed);
    assert(observer.source(1).audio_observed);
    assert(observer.source(0).low_band_energy_ratio > 0.50f);
    assert(observer.source(1).low_band_energy_ratio < 0.05f);
    assert(observer.source(0).low_band_energy_ratio
        > observer.source(1).low_band_energy_ratio);

    // Broad-band crowding is a deliberately coarse observation, not a masking
    // claim. Widely separated low/high sources should overlap much less than two
    // sources occupying the same broad spectral territory.
    const float disjoint_overlap = observer.scene().coarse_spectral_overlap;
    assert(disjoint_overlap < 0.25f);

    observer_type similar_observer{};
    std::array<vgmtooling::model::spatial_audio_lane_view, 2> similar_lanes{
        make_lane(low.data(), 21, 1),
        make_lane(low.data(), 22, 1),
    };
    const vgmtooling::model::spatial_source_block_view similar_block{
        similar_lanes.data(),
        similar_lanes.size(),
        frame_count,
    };
    assert(similar_observer.process(similar_block, sample_rate));
    assert(similar_observer.scene().coarse_spectral_overlap > 0.90f);
    assert(similar_observer.scene().coarse_spectral_overlap > disjoint_overlap + 0.60f);

    // The adaptive mix may react to this proxy only as crowding pressure. With
    // every other scene statistic held constant, more overlap must not make dry
    // objects or the shared field hazier. Global depth/height capacity remains
    // unchanged until a future explicit masking-aware separation control exists.
    auto clear_scene = similar_observer.scene();
    clear_scene.coarse_spectral_overlap = 0.0f;
    auto crowded_scene = clear_scene;
    crowded_scene.coarse_spectral_overlap = 1.0f;
    const auto clear_budget =
        vgmtooling::model::target_realtime_spatial_mix_budget(clear_scene);
    const auto crowded_budget =
        vgmtooling::model::target_realtime_spatial_mix_budget(crowded_scene);
    assert(crowded_budget.dry_width_scale < clear_budget.dry_width_scale);
    assert(crowded_budget.dry_diffuse_scale < clear_budget.dry_diffuse_scale);
    assert(crowded_budget.shared_wet_strength < clear_budget.shared_wet_strength);
    assert(crowded_budget.shared_wet_extent < clear_budget.shared_wet_extent);
    assert(crowded_budget.added_externalization_scale < clear_budget.added_externalization_scale);
    assert(crowded_budget.depth_scale == clear_budget.depth_scale);
    assert(crowded_budget.height_scale == clear_budget.height_scale);

    // Equal-amplitude equal-duration sources should divide observed energy
    // evenly, producing a concentration near 0.5 rather than inventing width.
    assert(std::fabs(observer.source(0).relative_energy - 0.5f) < 0.01f);
    assert(std::fabs(observer.source(1).relative_energy - 0.5f) < 0.01f);
    assert(std::fabs(observer.scene().energy_concentration - 0.5f) < 0.02f);

    // The persistent coarse profile is driven sample by sample, so callback
    // partitioning must not become a hidden spectral feature.
    observer_type split_observer{};
    constexpr std::size_t chunk_frames = 480;
    static_assert(frame_count % chunk_frames == 0);
    for (std::size_t offset = 0; offset < frame_count; offset += chunk_frames) {
        std::array<vgmtooling::model::spatial_audio_lane_view, 2> chunk_lanes{
            make_lane(low.data() + offset, 21, 1),
            make_lane(low.data() + offset, 22, 1),
        };
        const vgmtooling::model::spatial_source_block_view chunk{
            chunk_lanes.data(),
            chunk_lanes.size(),
            chunk_frames,
        };
        assert(split_observer.process(chunk, sample_rate));
    }
    assert(profile_distance(
        split_observer.source(0).coarse_band_energy_share,
        similar_observer.source(0).coarse_band_energy_share) < 1.0e-5f);
    assert(profile_distance(
        split_observer.source(1).coarse_band_energy_share,
        similar_observer.source(1).coarse_band_energy_share) < 1.0e-5f);
    assert(std::fabs(
        split_observer.scene().coarse_spectral_overlap
            - similar_observer.scene().coarse_spectral_overlap) < 1.0e-5f);

    // Shared effect audio stays an observed field contribution, not fictional
    // per-instrument wet stems. It is also excluded from the dry-source overlap
    // statistic even if its spectrum resembles a dry source exactly.
    lanes[0] = make_lane(low.data(), 31, 1);
    lanes[1] = make_lane(
        low.data(),
        32,
        1,
        vgmtooling::model::spatial_audio_lane_kind::shared_effect_return);
    assert(observer.process(two_lane_block, sample_rate));
    assert(std::fabs(observer.scene().shared_effect_energy_share - 0.5f) < 0.01f);
    assert(observer.scene().coarse_spectral_overlap == 0.0f);

    // Unavailable source samples are excluded rather than interpreted as zeros.
    std::array<std::uint8_t, frame_count> availability{};
    for (std::size_t frame = 0; frame < frame_count / 2; ++frame)
        availability[frame] = 1;
    lanes[0] = make_lane(low.data(), 3, 1,
        vgmtooling::model::spatial_audio_lane_kind::dry_source,
        availability.data());
    const vgmtooling::model::spatial_source_block_view one_lane_block{
        lanes.data(),
        1,
        frame_count,
    };
    assert(observer.process(one_lane_block, sample_rate));
    assert(observer.source(0).observed_frames == frame_count / 2);
    assert(std::fabs(observer.source(0).availability_fraction - 0.5f) < 1.0e-6f);

    // Bounded source identity age crosses ordinary block boundaries and resets
    // when the generation changes.
    const float first_age = observer.source(0).source_age_seconds;
    assert(observer.process(one_lane_block, sample_rate));
    const float second_age = observer.source(0).source_age_seconds;
    assert(second_age > first_age);
    lanes[0].evidence.generation = 2;
    assert(observer.process(one_lane_block, sample_rate));
    assert(observer.source(0).source_age_seconds < second_age);

    // A null source pointer means no acoustic observation. It must not become
    // false evidence of silence merely because the transport lane exists.
    lanes[0] = make_lane(nullptr, 9, 1);
    assert(observer.process(one_lane_block, sample_rate));
    assert(!observer.source(0).audio_observed);
    assert(observer.source(0).observed_frames == 0);
    assert(observer.source(0).rms == 0.0f);

    // Invalid processing input fails without advancing the persistent source
    // age. The next valid block therefore advances from the last valid state.
    lanes[0] = make_lane(low.data(), 11, 1);
    assert(observer.process(one_lane_block, sample_rate));
    const float age_before_invalid = observer.source(0).source_age_seconds;
    assert(!observer.process(one_lane_block, 0.0));
    assert(observer.process(one_lane_block, sample_rate));
    assert(observer.source(0).source_age_seconds > age_before_invalid);

    // A block-level acoustic summary may not silently merge two identities if
    // the driver reuses a physical lane inside that block.
    vgmtooling::model::spatial_source_evidence replacement = lanes[0].evidence;
    replacement.generation = 2;
    const vgmtooling::model::spatial_source_evidence_event identity_change{
        100,
        0,
        replacement,
    };
    const vgmtooling::model::spatial_source_block_view unstable_identity_block{
        lanes.data(),
        1,
        frame_count,
        &identity_change,
        1,
    };
    assert(!observer.process(unstable_identity_block, sample_rate));

    // Same-identity timed evidence is harmless to this acoustic observer; it
    // may represent routing or presentation changes handled by another layer.
    const vgmtooling::model::spatial_source_evidence_event stable_event{
        100,
        0,
        lanes[0].evidence,
    };
    const vgmtooling::model::spatial_source_block_view stable_identity_block{
        lanes.data(),
        1,
        frame_count,
        &stable_event,
        1,
    };
    assert(observer.process(stable_identity_block, sample_rate));

    // reset() clears public observations as well as private filter/history state.
    observer.reset();
    assert(observer.lane_count() == 0);
    assert(!observer.scene().audio_observed);
    assert(!observer.source(0).audio_observed);

    // A syntactically valid cutoff that exceeds Nyquist is rejected at process
    // time because its validity depends on the current audio sample rate.
    auto config = observer.config();
    config.coarse_upper_band_split_hz = 30000.0f;
    assert(observer.set_config(config));
    assert(!observer.process(one_lane_block, sample_rate));

    return 0;
}