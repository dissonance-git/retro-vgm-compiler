#pragma once

#include "spatial_source_bus.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class timed_spatial_source_bus_error : std::uint8_t {
    none = 0,
    base_bus_invalid,
    too_many_events,
    event_out_of_range,
    event_for_missing_source,
    invalid_event_evidence,
    unordered_events,
};

// Decorates a selected-source VGM bus with sparse delivered-clock route changes.
// Incoming event lane indices are canonical source indices. The base bus compacts
// absent sources, so this adapter remaps events to compact causal lanes without
// changing event order or frame timing.
template <std::size_t SourceCount,
          std::size_t MaxFrames = 8192,
          std::size_t MaxEvents = 256>
class timed_spatial_source_bus_storage {
    static_assert(SourceCount > 0, "spatial source count must be non-zero");

public:
    static constexpr std::size_t source_capacity = SourceCount;
    using source_array = std::array<source_family_stereo_view, source_capacity>;
    using evidence_array =
        std::array<vgmtooling::model::spatial_source_evidence, source_capacity>;
    using base_bus_type = spatial_source_bus_storage<source_capacity, MaxFrames>;

    void reset() noexcept {
        base_.reset();
        block_ = {};
        event_count_ = 0;
        valid_ = false;
        last_error_ = timed_spatial_source_bus_error::none;
    }

    bool build(
        const source_array& selected_sources,
        const evidence_array& initial_evidence,
        std::size_t frame_count,
        const vgmtooling::model::spatial_source_evidence_event* canonical_events,
        std::size_t canonical_event_count) noexcept
    {
        reset();
        if (canonical_event_count > MaxEvents)
            return fail(timed_spatial_source_bus_error::too_many_events);
        if (canonical_event_count != 0 && canonical_events == nullptr)
            return fail(timed_spatial_source_bus_error::event_out_of_range);
        if (!base_.build(selected_sources, initial_evidence, frame_count))
            return fail(timed_spatial_source_bus_error::base_bus_invalid);

        std::size_t previous_offset = 0;
        bool have_previous = false;
        for (std::size_t index = 0; index < canonical_event_count; ++index) {
            const auto& incoming = canonical_events[index];
            if (incoming.frame_offset >= frame_count || incoming.lane_index >= source_capacity)
                return fail(timed_spatial_source_bus_error::event_out_of_range);
            if (have_previous && incoming.frame_offset < previous_offset)
                return fail(timed_spatial_source_bus_error::unordered_events);

            std::size_t compact_index = base_.lane_count();
            for (std::size_t lane = 0; lane < base_.lane_count(); ++lane) {
                if (base_.canonical_source_index(lane) == incoming.lane_index) {
                    compact_index = lane;
                    break;
                }
            }
            if (compact_index == base_.lane_count())
                return fail(timed_spatial_source_bus_error::event_for_missing_source);

            auto evidence = incoming.evidence;
            if (evidence.family != vgmtooling::model::spatial_source_family::vgm
                || evidence.source_id == 0 || !evidence.stereo_route.present)
                return fail(timed_spatial_source_bus_error::invalid_event_evidence);

            // Selected isolated PCM already carries native route gain sample by
            // sample. Timed evidence updates presentation state only.
            evidence.stereo_route.gain_preapplied = true;
            events_[index].frame_offset = incoming.frame_offset;
            events_[index].lane_index = compact_index;
            events_[index].evidence = evidence;
            previous_offset = incoming.frame_offset;
            have_previous = true;
        }

        event_count_ = canonical_event_count;
        block_ = base_.block();
        block_.evidence_events = event_count_ == 0 ? nullptr : events_.data();
        block_.evidence_event_count = event_count_;
        valid_ = true;
        last_error_ = timed_spatial_source_bus_error::none;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t lane_count() const noexcept { return valid_ ? base_.lane_count() : 0; }
    std::size_t frame_count() const noexcept { return valid_ ? base_.frame_count() : 0; }
    timed_spatial_source_bus_error last_error() const noexcept { return last_error_; }
    std::size_t event_count() const noexcept { return valid_ ? event_count_ : 0; }

    const vgmtooling::model::spatial_source_block_view& block() const noexcept {
        return block_;
    }

    std::size_t canonical_source_index(std::size_t lane_index) const noexcept {
        return valid_ ? base_.canonical_source_index(lane_index) : source_capacity;
    }

    const base_bus_type& base_bus() const noexcept { return base_; }

private:
    bool fail(timed_spatial_source_bus_error error) noexcept {
        block_ = {};
        event_count_ = 0;
        valid_ = false;
        last_error_ = error;
        return false;
    }

    base_bus_type base_{};
    std::array<vgmtooling::model::spatial_source_evidence_event, MaxEvents> events_{};
    vgmtooling::model::spatial_source_block_view block_{};
    std::size_t event_count_ = 0;
    bool valid_ = false;
    timed_spatial_source_bus_error last_error_ = timed_spatial_source_bus_error::none;
};

} // namespace gameaudio::vgm
