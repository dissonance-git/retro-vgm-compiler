#pragma once

#include "genesis_spatial_source_bus.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class genesis_timed_spatial_source_bus_error : std::uint8_t {
    none = 0,
    base_bus_invalid,
    too_many_events,
    event_out_of_range,
    event_for_missing_source,
    invalid_event_evidence,
    unordered_events,
};

// Decorates the selected-source Genesis bus with sparse route/evidence changes.
// Incoming event lane_index values are canonical Genesis source indices. The
// base bus compacts absent source families, so this adapter remaps every event to
// the compact lane index without changing ordering or frame timing.
template <std::size_t MaxFrames = 8192, std::size_t MaxEvents = 256>
class genesis_timed_spatial_source_bus_storage {
public:
    static constexpr std::size_t source_capacity = genesis_recomposition_source_count;
    using source_array = std::array<genesis_stereo_source_view, source_capacity>;
    using evidence_array =
        std::array<vgmtooling::model::spatial_source_evidence, source_capacity>;

    void reset() noexcept {
        base_.reset();
        block_ = {};
        event_count_ = 0;
        valid_ = false;
        last_error_ = genesis_timed_spatial_source_bus_error::none;
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
            return fail(genesis_timed_spatial_source_bus_error::too_many_events);
        if (canonical_event_count != 0 && canonical_events == nullptr)
            return fail(genesis_timed_spatial_source_bus_error::event_out_of_range);
        if (!base_.build(selected_sources, initial_evidence, frame_count))
            return fail(genesis_timed_spatial_source_bus_error::base_bus_invalid);

        std::size_t previous_offset = 0;
        bool have_previous = false;
        for (std::size_t index = 0; index < canonical_event_count; ++index) {
            const auto& incoming = canonical_events[index];
            if (incoming.frame_offset >= frame_count || incoming.lane_index >= source_capacity)
                return fail(genesis_timed_spatial_source_bus_error::event_out_of_range);
            if (have_previous && incoming.frame_offset < previous_offset)
                return fail(genesis_timed_spatial_source_bus_error::unordered_events);

            std::size_t compact_index = base_.lane_count();
            for (std::size_t lane = 0; lane < base_.lane_count(); ++lane) {
                if (base_.canonical_source_index(lane) == incoming.lane_index) {
                    compact_index = lane;
                    break;
                }
            }
            if (compact_index == base_.lane_count())
                return fail(genesis_timed_spatial_source_bus_error::event_for_missing_source);

            auto evidence = incoming.evidence;
            if (evidence.family != vgmtooling::model::spatial_source_family::vgm
                || evidence.source_id == 0 || !evidence.stereo_route.present)
                return fail(genesis_timed_spatial_source_bus_error::invalid_event_evidence);

            // The selected isolated PCM already carries the exact sample-wise
            // authored route trajectory. Events update presentation evidence;
            // Omniphony must not multiply the native gain into the PCM again.
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
        last_error_ = genesis_timed_spatial_source_bus_error::none;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    genesis_timed_spatial_source_bus_error last_error() const noexcept { return last_error_; }
    std::size_t event_count() const noexcept { return valid_ ? event_count_ : 0; }

    const vgmtooling::model::spatial_source_block_view& block() const noexcept {
        return block_;
    }

    const genesis_spatial_source_bus_storage<MaxFrames>& base_bus() const noexcept {
        return base_;
    }

private:
    bool fail(genesis_timed_spatial_source_bus_error error) noexcept {
        block_ = {};
        event_count_ = 0;
        valid_ = false;
        last_error_ = error;
        return false;
    }

    genesis_spatial_source_bus_storage<MaxFrames> base_{};
    std::array<vgmtooling::model::spatial_source_evidence_event, MaxEvents> events_{};
    vgmtooling::model::spatial_source_block_view block_{};
    std::size_t event_count_ = 0;
    bool valid_ = false;
    genesis_timed_spatial_source_bus_error last_error_ =
        genesis_timed_spatial_source_bus_error::none;
};

} // namespace gameaudio::vgm
