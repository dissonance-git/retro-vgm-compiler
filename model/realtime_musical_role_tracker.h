#pragma once

#include "realtime_musical_role_hypothesis.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace vgmtooling::model {

// Runtime identity used only for carrying musical-role state through time. A
// sufficiently supported persistent part outranks the bounded source episode;
// otherwise the source ID + generation remains the safe continuity boundary.
enum class realtime_role_identity_kind : std::uint8_t {
    unavailable = 0,
    source_episode,
    persistent_part,
};

struct realtime_role_identity {
    realtime_role_identity_kind kind = realtime_role_identity_kind::unavailable;
    std::uint64_t id = 0;
    std::uint64_t generation = 0;
};

constexpr bool operator==(
    const realtime_role_identity& left,
    const realtime_role_identity& right) noexcept
{
    return left.kind == right.kind && left.id == right.id
        && left.generation == right.generation;
}

struct realtime_role_tracker_policy {
    float persistent_part_min_confidence = 0.75f;
    float score_smoothing_seconds = 0.25f;
    float confidence_smoothing_seconds = 0.40f;
    float missing_confidence_decay_seconds = 1.50f;
    float max_hold_seconds = 4.0f;
};

constexpr realtime_role_identity realtime_role_identity_for(
    const spatial_source_evidence& source,
    float persistent_part_min_confidence = 0.75f) noexcept
{
    if (source.persistent_part_present && source.persistent_part_id != 0 &&
        source.persistent_part_confidence >= persistent_part_min_confidence) {
        // persistent_part_id is expected to be playback-episode scoped by the
        // producer of that evidence. Do not derive it from a physical slot.
        return {
            realtime_role_identity_kind::persistent_part,
            source.persistent_part_id,
            0,
        };
    }

    if (source.source_id != 0) {
        return {
            realtime_role_identity_kind::source_episode,
            source.source_id,
            source.generation,
        };
    }

    return {};
}

template <std::size_t Capacity = 128>
class realtime_musical_role_tracker {
public:
    static_assert(Capacity > 0, "role tracker requires at least one state slot");

    void reset() noexcept {
        entries_ = {};
        stream_seconds_ = 0.0;
        block_serial_ = 0;
    }

    const realtime_role_tracker_policy& policy() const noexcept {
        return policy_;
    }

    bool set_policy(realtime_role_tracker_policy policy) noexcept {
        const realtime_role_tracker_policy previous = policy_;
        policy_ = policy;
        if (!valid_policy()) {
            policy_ = previous;
            return false;
        }
        return true;
    }

    double stream_seconds() const noexcept {
        return stream_seconds_;
    }

    // Musical time advances exactly once per audio block, regardless of how
    // many source lanes are observed in that block. This prevents a 19-source
    // QSound block from aging the state nineteen times faster than the audio.
    bool advance_block(std::size_t frame_count, double sample_rate) noexcept {
        if (!valid_policy() || !std::isfinite(sample_rate) || sample_rate <= 0.0)
            return false;

        stream_seconds_ += static_cast<double>(frame_count) / sample_rate;
        if (block_serial_ != std::numeric_limits<std::uint64_t>::max())
            ++block_serial_;
        return true;
    }

    // Observe any number of sources after advance_block(). All observations in
    // that block share one stream time. No heap allocation or physical-lane key
    // is used. When capacity is exhausted, the least-recently observed entry is
    // deterministically replaced.
    void observe(
        const spatial_source_evidence& source,
        const realtime_musical_role_hypotheses& incoming) noexcept
    {
        const realtime_role_identity key = realtime_role_identity_for(
            source,
            policy_.persistent_part_min_confidence);
        if (key.kind == realtime_role_identity_kind::unavailable)
            return;

        // Copy scalar key fields before a possible entry reset. This also keeps
        // aggressive dangling-pointer diagnostics happy when Capacity == 1.
        const realtime_role_identity_kind key_kind = key.kind;
        const std::uint64_t key_id = key.id;
        const std::uint64_t key_generation = key.generation;

        const std::size_t index = find_or_allocate(key);
        entry& state = entries_[index];

        if (!state.valid || !(state.identity == key)) {
            state = entry{};
            state.valid = true;
            state.identity.kind = key_kind;
            state.identity.id = key_id;
            state.identity.generation = key_generation;
            state.roles = incoming;
        } else {
            const float elapsed_seconds = static_cast<float>(std::max(
                0.0,
                stream_seconds_ - state.last_seen_seconds));
            blend_roles(state.roles, incoming, elapsed_seconds);
        }

        state.last_seen_seconds = stream_seconds_;
        state.last_seen_serial = block_serial_;
    }

    bool lookup(
        const spatial_source_evidence& source,
        realtime_musical_role_hypotheses& output) const noexcept
    {
        const realtime_role_identity key = realtime_role_identity_for(
            source,
            policy_.persistent_part_min_confidence);
        if (key.kind == realtime_role_identity_kind::unavailable)
            return false;

        for (const entry& state : entries_) {
            if (!state.valid || !(state.identity == key))
                continue;
            if (stream_seconds_ - state.last_seen_seconds > policy_.max_hold_seconds)
                return false;
            output = state.roles;
            return true;
        }
        return false;
    }

private:
    struct entry {
        bool valid = false;
        realtime_role_identity identity{};
        realtime_musical_role_hypotheses roles{};
        double last_seen_seconds = 0.0;
        std::uint64_t last_seen_serial = 0;
    };

    bool valid_policy() const noexcept {
        return std::isfinite(policy_.persistent_part_min_confidence)
            && policy_.persistent_part_min_confidence >= 0.0f
            && policy_.persistent_part_min_confidence <= 1.0f
            && std::isfinite(policy_.score_smoothing_seconds)
            && policy_.score_smoothing_seconds >= 0.0f
            && std::isfinite(policy_.confidence_smoothing_seconds)
            && policy_.confidence_smoothing_seconds >= 0.0f
            && std::isfinite(policy_.missing_confidence_decay_seconds)
            && policy_.missing_confidence_decay_seconds > 0.0f
            && std::isfinite(policy_.max_hold_seconds)
            && policy_.max_hold_seconds >= 0.0f;
    }

    static float smoothing_alpha(float elapsed_seconds, float time_constant) noexcept {
        if (time_constant <= 0.0f)
            return 1.0f;
        return 1.0f - std::exp(-elapsed_seconds / time_constant);
    }

    void blend_hypothesis(
        realtime_role_hypothesis& current,
        const realtime_role_hypothesis& incoming,
        float elapsed_seconds) const noexcept
    {
        const float score_alpha = smoothing_alpha(
            elapsed_seconds,
            policy_.score_smoothing_seconds);
        const float confidence_alpha = smoothing_alpha(
            elapsed_seconds,
            policy_.confidence_smoothing_seconds);

        if (incoming.confidence > 0.0f) {
            const float trust = clamp_unit_interval(incoming.confidence);
            const float effective_score_alpha = score_alpha * trust;
            current.score = clamp_unit_interval(
                current.score
                    + effective_score_alpha * (incoming.score - current.score));
            current.confidence = clamp_unit_interval(
                current.confidence
                    + confidence_alpha * (incoming.confidence - current.confidence));
            current.cues |= incoming.cues;
            return;
        }

        // If a still-observed source temporarily has no support for this role,
        // confidence relaxes rather than snapping to zero. Entirely absent
        // sources are handled separately by max_hold_seconds.
        const float decay = std::exp(
            -elapsed_seconds / policy_.missing_confidence_decay_seconds);
        current.confidence = clamp_unit_interval(current.confidence * decay);
    }

    void blend_roles(
        realtime_musical_role_hypotheses& current,
        const realtime_musical_role_hypotheses& incoming,
        float elapsed_seconds) const noexcept
    {
        blend_hypothesis(current.foundation, incoming.foundation, elapsed_seconds);
        blend_hypothesis(current.foreground, incoming.foreground, elapsed_seconds);
        blend_hypothesis(
            current.transient_accent,
            incoming.transient_accent,
            elapsed_seconds);
        blend_hypothesis(
            current.environmental_layer,
            incoming.environmental_layer,
            elapsed_seconds);
    }

    std::size_t find_or_allocate(realtime_role_identity key) const noexcept {
        std::size_t oldest_index = 0;
        std::uint64_t oldest_serial = std::numeric_limits<std::uint64_t>::max();

        for (std::size_t index = 0; index < Capacity; ++index) {
            if (entries_[index].valid && entries_[index].identity == key)
                return index;
            if (!entries_[index].valid)
                return index;
            if (entries_[index].last_seen_serial < oldest_serial) {
                oldest_serial = entries_[index].last_seen_serial;
                oldest_index = index;
            }
        }

        return oldest_index;
    }

    realtime_role_tracker_policy policy_{};
    std::array<entry, Capacity> entries_{};
    double stream_seconds_ = 0.0;
    std::uint64_t block_serial_ = 0;
};

} // namespace vgmtooling::model
