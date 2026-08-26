#pragma once

#include "genesis_enhanced_recomposition.h"
#include "vgm_command_event.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

constexpr std::size_t genesis_episode_depth_slot_count = 6u;
constexpr float genesis_episode_default_depth = 0.5f;

constexpr std::array<float, genesis_episode_depth_slot_count>
    genesis_episode_depth_slots{{
        1.0f / 12.0f,
        3.0f / 12.0f,
        5.0f / 12.0f,
        7.0f / 12.0f,
        9.0f / 12.0f,
        11.0f / 12.0f,
    }};

struct genesis_source_episode_assignment {
    std::uint32_t generation = 0;
    std::uint8_t depth_slot = 0;
    bool assigned = false;
    bool active = false;
};

struct genesis_source_episode_event {
    std::size_t frame_offset = 0;
    std::size_t source_index = 0;
    float depth = genesis_episode_default_depth;
    std::uint32_t generation = 0;
};

template <std::size_t MaxEvents>
struct genesis_source_episode_block {
    std::array<float, genesis_recomposition_source_count> initial_depth{};
    std::array<genesis_source_episode_event, MaxEvents> events{};
    std::size_t event_count = 0;
    bool valid = false;
};

// Role-free delivery-clock allocator for YM2612 + SN76489 source episodes.
//
// A new episode takes the least-occupied horizontal depth slot with a rotating
// tie-breaker. Hardware channel number never determines speaker role. Key-off
// releases allocator occupancy but deliberately leaves the last delivered depth
// in place so an FM release tail never snaps to another speaker.
//
// Seek replay can span minutes, so begin_replay() rebuilds allocator state
// without queueing historical transitions. end_replay() rebases the current
// producer assignments as delivered state at the seek destination.
template <std::size_t QueueCapacity = 2048, std::size_t MaxBlockEvents = 256>
class genesis_source_episode_transport {
    static_assert(QueueCapacity > 0, "episode queue capacity must be non-zero");
    static_assert(MaxBlockEvents > 0, "episode event capacity must be non-zero");

    struct queued_transition {
        std::uint64_t ordinal = 0;
        std::size_t source_index = 0;
        std::uint8_t depth_slot = 0;
        std::uint32_t generation = 0;
    };

public:
    using block_type = genesis_source_episode_block<MaxBlockEvents>;

    genesis_source_episode_transport() noexcept { clear_state(false); }

    void reset() noexcept {
        const bool replay = replay_mode_;
        clear_state(replay);
    }

    void begin_replay() noexcept {
        replay_mode_ = true;
        clear_state(true);
    }

    void end_replay() noexcept {
        replay_mode_ = false;
        queue_head_ = 0u;
        queue_size_ = 0u;
        have_last_ordinal_ = false;
        delivered_ = producer_;
    }

    bool valid() const noexcept { return valid_; }
    bool replay_mode() const noexcept { return replay_mode_; }

    const genesis_source_episode_assignment& producer_assignment(
        std::size_t source_index) const noexcept
    {
        return producer_[source_index < producer_.size() ? source_index : 0u];
    }

    bool observe(const command_event& event, std::uint64_t absolute_sample) noexcept {
        if (!valid_)
            return false;

        if (event.kind == command_event_kind::reset) {
            reset();
            return true;
        }
        if (event.kind != command_event_kind::command || event.payload == nullptr)
            return true;

        if (event.command == 0x52u && event.payload_size >= 2u) {
            const std::uint8_t reg = event.payload[0];
            const std::uint8_t data = event.payload[1];

            if (reg == 0x28u) {
                const std::uint8_t code = static_cast<std::uint8_t>(data & 0x07u);
                std::size_t channel = ym_key_mask_.size();
                if (code <= 2u)
                    channel = code;
                else if (code >= 4u && code <= 6u)
                    channel = static_cast<std::size_t>(code - 1u);

                if (channel < ym_key_mask_.size()) {
                    const std::uint8_t old_mask = ym_key_mask_[channel];
                    const std::uint8_t new_mask =
                        static_cast<std::uint8_t>((data >> 4u) & 0x0Fu);
                    if (old_mask == 0u && new_mask != 0u) {
                        if (!begin_episode(channel, absolute_sample))
                            return false;
                    } else if (old_mask != 0u && new_mask == 0u) {
                        end_episode(channel);
                    }
                    ym_key_mask_[channel] = new_mask;
                }
                return true;
            }

            if (reg == 0x2Bu) {
                const bool enabled = (data & 0x80u) != 0u;
                constexpr std::size_t dac_source = static_cast<std::size_t>(
                    genesis_recomposition_source::ym2612_dac);
                if (!dac_active_ && enabled) {
                    if (!begin_episode(dac_source, absolute_sample))
                        return false;
                } else if (dac_active_ && !enabled) {
                    end_episode(dac_source);
                }
                dac_active_ = enabled;
                return true;
            }
        }

        if (event.command == 0x50u && event.payload_size >= 1u) {
            observe_psg(event.payload[0], absolute_sample);
            return valid_;
        }

        return true;
    }

    bool prepare_delivered_block(
        std::uint64_t block_start,
        std::size_t frame_count,
        block_type& output) noexcept
    {
        output = {};
        if (!valid_ || replay_mode_ || frame_count == 0)
            return false;
        if (frame_count > static_cast<std::size_t>(
                std::numeric_limits<std::uint64_t>::max() - block_start))
            return fail_closed();

        for (std::size_t source = 0; source < delivered_.size(); ++source)
            output.initial_depth[source] = depth_for(delivered_[source]);

        const std::uint64_t block_end =
            block_start + static_cast<std::uint64_t>(frame_count);

        while (queue_size_ != 0u) {
            const queued_transition transition = queue_[queue_head_];
            if (transition.ordinal < block_start)
                return fail_closed();
            if (transition.ordinal >= block_end)
                break;

            const std::size_t frame_offset =
                static_cast<std::size_t>(transition.ordinal - block_start);
            apply_delivered_transition(transition);

            if (frame_offset == 0u) {
                output.initial_depth[transition.source_index] =
                    genesis_episode_depth_slots[transition.depth_slot];
            } else {
                if (output.event_count >= output.events.size())
                    return fail_closed();
                output.events[output.event_count++] = {
                    frame_offset,
                    transition.source_index,
                    genesis_episode_depth_slots[transition.depth_slot],
                    transition.generation,
                };
            }

            queue_head_ = (queue_head_ + 1u) % QueueCapacity;
            --queue_size_;
        }

        output.valid = true;
        return true;
    }

private:
    static float depth_for(
        const genesis_source_episode_assignment& assignment) noexcept
    {
        if (!assignment.assigned
            || assignment.depth_slot >= genesis_episode_depth_slots.size())
            return genesis_episode_default_depth;
        return genesis_episode_depth_slots[assignment.depth_slot];
    }

    void clear_state(bool replay) noexcept {
        producer_ = {};
        delivered_ = {};
        slot_occupancy_.fill(0u);
        ym_key_mask_.fill(0u);
        psg_attenuation_.fill(0x0Fu);
        psg_latched_channel_ = 0u;
        psg_latched_volume_ = false;
        dac_active_ = false;
        slot_cursor_ = 0u;
        queue_head_ = 0u;
        queue_size_ = 0u;
        have_last_ordinal_ = false;
        last_ordinal_ = 0u;
        valid_ = true;
        replay_mode_ = replay;
    }

    std::uint8_t choose_depth_slot() noexcept {
        std::uint8_t best = slot_cursor_;
        std::uint8_t best_occupancy = slot_occupancy_[best];
        for (std::size_t offset = 1u;
             offset < genesis_episode_depth_slot_count;
             ++offset) {
            const std::uint8_t slot = static_cast<std::uint8_t>(
                (static_cast<std::size_t>(slot_cursor_) + offset)
                % genesis_episode_depth_slot_count);
            if (slot_occupancy_[slot] < best_occupancy) {
                best = slot;
                best_occupancy = slot_occupancy_[slot];
            }
        }
        slot_cursor_ = static_cast<std::uint8_t>(
            (static_cast<std::size_t>(best) + 1u)
            % genesis_episode_depth_slot_count);
        return best;
    }

    bool begin_episode(std::size_t source_index, std::uint64_t ordinal) noexcept {
        if (source_index >= producer_.size())
            return false;

        auto& assignment = producer_[source_index];
        if (assignment.active)
            release_occupancy(assignment.depth_slot);

        const std::uint8_t slot = choose_depth_slot();
        if (slot_occupancy_[slot] != std::numeric_limits<std::uint8_t>::max())
            ++slot_occupancy_[slot];

        ++assignment.generation;
        if (assignment.generation == 0u)
            ++assignment.generation;
        assignment.depth_slot = slot;
        assignment.assigned = true;
        assignment.active = true;

        const queued_transition transition{
            ordinal, source_index, slot, assignment.generation};

        if (replay_mode_) {
            apply_delivered_transition(transition);
            return true;
        }
        return push_transition(transition);
    }

    void end_episode(std::size_t source_index) noexcept {
        if (source_index >= producer_.size())
            return;
        auto& assignment = producer_[source_index];
        if (assignment.active)
            release_occupancy(assignment.depth_slot);
        assignment.active = false;
    }

    void release_occupancy(std::uint8_t slot) noexcept {
        if (slot < slot_occupancy_.size() && slot_occupancy_[slot] != 0u)
            --slot_occupancy_[slot];
    }

    bool push_transition(const queued_transition& transition) noexcept {
        if (queue_size_ >= QueueCapacity)
            return fail_closed();
        if (have_last_ordinal_ && transition.ordinal < last_ordinal_)
            return fail_closed();

        queue_[(queue_head_ + queue_size_) % QueueCapacity] = transition;
        ++queue_size_;
        last_ordinal_ = transition.ordinal;
        have_last_ordinal_ = true;
        return true;
    }

    void apply_delivered_transition(
        const queued_transition& transition) noexcept
    {
        if (transition.source_index >= delivered_.size())
            return;
        auto& assignment = delivered_[transition.source_index];
        assignment.generation = transition.generation;
        assignment.depth_slot = transition.depth_slot;
        assignment.assigned = true;
    }

    void observe_psg(std::uint8_t data, std::uint64_t ordinal) noexcept {
        if ((data & 0x80u) != 0u) {
            psg_latched_channel_ =
                static_cast<std::uint8_t>((data >> 5u) & 0x03u);
            psg_latched_volume_ = (data & 0x10u) != 0u;
            if (psg_latched_volume_)
                set_psg_attenuation(
                    psg_latched_channel_,
                    static_cast<std::uint8_t>(data & 0x0Fu),
                    ordinal);
            return;
        }

        if (psg_latched_volume_)
            set_psg_attenuation(
                psg_latched_channel_,
                static_cast<std::uint8_t>(data & 0x0Fu),
                ordinal);
    }

    void set_psg_attenuation(
        std::uint8_t channel,
        std::uint8_t attenuation,
        std::uint64_t ordinal) noexcept
    {
        if (channel >= psg_attenuation_.size())
            return;

        const bool was_active = psg_attenuation_[channel] < 0x0Fu;
        const bool is_active = attenuation < 0x0Fu;
        psg_attenuation_[channel] = attenuation;

        const std::size_t source_index =
            static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0)
            + static_cast<std::size_t>(channel);

        if (!was_active && is_active) {
            if (!begin_episode(source_index, ordinal))
                (void)fail_closed();
        } else if (was_active && !is_active) {
            end_episode(source_index);
        }
    }

    bool fail_closed() noexcept {
        valid_ = false;
        queue_head_ = 0u;
        queue_size_ = 0u;
        have_last_ordinal_ = false;
        return false;
    }

    std::array<
        genesis_source_episode_assignment,
        genesis_recomposition_source_count> producer_{};
    std::array<
        genesis_source_episode_assignment,
        genesis_recomposition_source_count> delivered_{};
    std::array<
        std::uint8_t,
        genesis_episode_depth_slot_count> slot_occupancy_{};

    std::array<std::uint8_t, 6> ym_key_mask_{};
    std::array<std::uint8_t, 4> psg_attenuation_{};
    std::uint8_t psg_latched_channel_ = 0u;
    bool psg_latched_volume_ = false;
    bool dac_active_ = false;

    std::uint8_t slot_cursor_ = 0u;
    std::array<queued_transition, QueueCapacity> queue_{};
    std::size_t queue_head_ = 0u;
    std::size_t queue_size_ = 0u;
    bool have_last_ordinal_ = false;
    std::uint64_t last_ordinal_ = 0u;
    bool valid_ = true;
    bool replay_mode_ = false;
};

} // namespace gameaudio::vgm
