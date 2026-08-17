#pragma once

#include "genesis_selected_source_queue.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

enum class genesis_selected_source_block_error : std::uint8_t {
    none = 0,
    invalid_frames,
    source_queue_invalid,
    ordinal_mismatch,
    topology_changed,
    inexact_source,
    sample_out_of_range,
    no_sources,
};

// Fixed-capacity bridge from PlayerA's ordinal selected-source queue to one
// delivered foobar block. The ordinary stereo block already exists before this
// adapter is consulted; failure therefore disables source-aware Spatial for the
// affected transport generation without damaging audible reference/enhanced
// stereo fallback.
template <std::size_t MaxFrames = 8192>
class genesis_selected_source_block_storage {
    static_assert(MaxFrames > 0, "selected source block capacity must be non-zero");

public:
    static constexpr std::size_t source_count = genesis_recomposition_source_count;
    using source_array = std::array<genesis_stereo_source_view, source_count>;

    void reset() noexcept {
        sources_ = {};
        present_.fill(false);
        frame_count_ = 0;
        first_ordinal_ = 0;
        valid_ = false;
        last_error_ = genesis_selected_source_block_error::none;
    }

    template <std::size_t QueueCapacity>
    bool consume(
        genesis_selected_source_queue<QueueCapacity>& queue,
        std::uint64_t first_ordinal,
        std::size_t frame_count) noexcept
    {
        reset();
        if (frame_count == 0 || frame_count > MaxFrames)
            return fail(genesis_selected_source_block_error::invalid_frames, nullptr);
        if (!queue.valid())
            return fail(genesis_selected_source_block_error::source_queue_invalid, nullptr);
        if (frame_count > static_cast<std::size_t>(
                std::numeric_limits<std::uint64_t>::max() - first_ordinal))
            return fail(genesis_selected_source_block_error::invalid_frames, &queue);

        std::array<bool, source_count> topology{};
        bool topology_initialized = false;
        bool any_source = false;

        for (std::size_t frame = 0; frame < frame_count; ++frame) {
            const std::uint64_t ordinal = first_ordinal + static_cast<std::uint64_t>(frame);
            genesis_selected_source_frame selected{};
            if (!queue.pop_expected(ordinal, selected))
                return fail(genesis_selected_source_block_error::ordinal_mismatch, &queue);

            std::array<bool, source_count> frame_topology{};
            for (std::size_t source_index = 0; source_index < source_count; ++source_index) {
                const auto& source = selected.source[source_index];
                frame_topology[source_index] = source.present;
                if (!source.present)
                    continue;
                any_source = true;
                if (!source.exact)
                    return fail(genesis_selected_source_block_error::inexact_source, &queue);
                if (!store_sample(source_index, frame, source.left, source.right))
                    return fail(genesis_selected_source_block_error::sample_out_of_range, &queue);
            }

            if (!topology_initialized) {
                topology = frame_topology;
                topology_initialized = true;
            } else if (topology != frame_topology) {
                return fail(genesis_selected_source_block_error::topology_changed, &queue);
            }
        }

        if (!any_source)
            return fail(genesis_selected_source_block_error::no_sources, &queue);

        present_ = topology;
        for (std::size_t source_index = 0; source_index < source_count; ++source_index) {
            if (!present_[source_index])
                continue;
            sources_[source_index] = {
                left_[source_index].data(),
                right_[source_index].data(),
                true,
            };
        }

        first_ordinal_ = first_ordinal;
        frame_count_ = frame_count;
        valid_ = true;
        last_error_ = genesis_selected_source_block_error::none;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    std::uint64_t first_ordinal() const noexcept { return valid_ ? first_ordinal_ : 0; }
    std::size_t frame_count() const noexcept { return valid_ ? frame_count_ : 0; }
    genesis_selected_source_block_error last_error() const noexcept { return last_error_; }
    const source_array& sources() const noexcept { return sources_; }
    bool source_present(std::size_t index) const noexcept {
        return valid_ && index < source_count && present_[index];
    }

private:
    static bool representable_float(double value) noexcept {
        if (!std::isfinite(value))
            return false;
        const double limit = static_cast<double>(std::numeric_limits<float>::max());
        return value >= -limit && value <= limit;
    }

    bool store_sample(
        std::size_t source_index,
        std::size_t frame,
        double left,
        double right) noexcept
    {
        if (!representable_float(left) || !representable_float(right))
            return false;
        const float left_value = static_cast<float>(left);
        const float right_value = static_cast<float>(right);
        if (!std::isfinite(left_value) || !std::isfinite(right_value))
            return false;
        left_[source_index][frame] = left_value;
        right_[source_index][frame] = right_value;
        return true;
    }

    template <std::size_t QueueCapacity>
    bool fail(
        genesis_selected_source_block_error error,
        genesis_selected_source_queue<QueueCapacity>* queue) noexcept
    {
        if (queue != nullptr)
            queue->fail_closed_state();
        sources_ = {};
        present_.fill(false);
        frame_count_ = 0;
        first_ordinal_ = 0;
        valid_ = false;
        last_error_ = error;
        return false;
    }

    // Non-queue failures use this overload so callers can reject parameters
    // without invalidating an otherwise coherent source queue.
    bool fail(genesis_selected_source_block_error error, std::nullptr_t) noexcept {
        sources_ = {};
        present_.fill(false);
        frame_count_ = 0;
        first_ordinal_ = 0;
        valid_ = false;
        last_error_ = error;
        return false;
    }

    std::array<std::array<float, MaxFrames>, source_count> left_{};
    std::array<std::array<float, MaxFrames>, source_count> right_{};
    source_array sources_{};
    std::array<bool, source_count> present_{};
    std::uint64_t first_ordinal_ = 0;
    std::size_t frame_count_ = 0;
    bool valid_ = false;
    genesis_selected_source_block_error last_error_ = genesis_selected_source_block_error::none;
};

} // namespace gameaudio::vgm
