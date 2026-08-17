#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace foobar_vgm::source_audio {

struct studio_transport_input_frame {
    std::uint64_t destination_ordinal = 0;
    std::int32_t protected_left = 0;
    std::int32_t protected_right = 0;
    std::int64_t exact_fm_left = 0;
    std::int64_t exact_fm_right = 0;
    bool await_studio_fm = false;
};

struct studio_transport_output_frame {
    std::uint64_t destination_ordinal = 0;
    std::int32_t left = 0;
    std::int32_t right = 0;
    bool used_studio_fm = false;
};

// Whole-host-frame queue for non-causal Studio FM reconstruction.
//
// protected_* is already the authoritative PlayerA frame, optionally amended by
// independently valid non-FM descendants such as PSG. exact_fm_* is the exact
// six-channel FM contribution at that same ordinal. Once Studio FM for the
// ordinal arrives, only that family is replaced:
//
//   final = protected + studio_fm - exact_fm
//
// Frames that cannot ever obtain symmetric FIR support are inserted/finalized
// as reference. The head can leave only when final, so DAC/PSG/unrelated chips
// inherit exactly the same decoder lookahead as FM without being resampled.
template <std::size_t Capacity>
class studio_frame_transport {
    static_assert(Capacity > 0, "Studio frame transport capacity must be non-zero.");

    enum class state : std::uint8_t {
        waiting_studio,
        final_reference,
        final_studio,
    };

    struct entry {
        studio_transport_input_frame input{};
        std::int32_t final_left = 0;
        std::int32_t final_right = 0;
        state status = state::final_reference;
    };

public:
    void reset() noexcept {
        head_ = 0;
        count_ = 0;
        waiting_ = 0;
        have_next_ordinal_ = false;
        next_destination_ordinal_ = 0;
        valid_ = true;
    }

    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    [[nodiscard]] bool full() const noexcept { return count_ == Capacity; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] std::size_t waiting_frames() const noexcept { return waiting_; }

    [[nodiscard]] std::size_t contiguous_final_frames() const noexcept {
        if (!valid_)
            return 0;
        std::size_t ready = 0;
        while (ready < count_) {
            const entry& current = entries_[(head_ + ready) % Capacity];
            if (current.status == state::waiting_studio)
                break;
            ++ready;
        }
        return ready;
    }

    bool push(const studio_transport_input_frame& input) noexcept {
        if (!valid_ || full()) {
            valid_ = false;
            return false;
        }
        if (have_next_ordinal_) {
            if (input.destination_ordinal != next_destination_ordinal_) {
                valid_ = false;
                return false;
            }
        } else {
            have_next_ordinal_ = true;
        }
        if (input.destination_ordinal == std::numeric_limits<std::uint64_t>::max()) {
            valid_ = false;
            return false;
        }

        entry& dst = entries_[(head_ + count_) % Capacity];
        dst.input = input;
        dst.final_left = input.protected_left;
        dst.final_right = input.protected_right;
        dst.status = input.await_studio_fm
            ? state::waiting_studio
            : state::final_reference;
        if (input.await_studio_fm)
            ++waiting_;
        ++count_;
        next_destination_ordinal_ = input.destination_ordinal + 1u;
        return true;
    }

    bool apply_studio_fm(
        std::uint64_t destination_ordinal,
        std::int64_t studio_fm_left,
        std::int64_t studio_fm_right) noexcept {
        if (!valid_ || count_ == 0)
            return false;

        const std::uint64_t first = entries_[head_].input.destination_ordinal;
        if (destination_ordinal < first)
            return false;
        const std::uint64_t offset64 = destination_ordinal - first;
        if (offset64 >= count_)
            return false;
        const std::size_t offset = static_cast<std::size_t>(offset64);
        entry& target = entries_[(head_ + offset) % Capacity];
        if (target.input.destination_ordinal != destination_ordinal
            || target.status != state::waiting_studio)
            return false;

        std::int64_t left = 0;
        std::int64_t right = 0;
        if (!candidate_value(
                target.input.protected_left,
                studio_fm_left,
                target.input.exact_fm_left,
                left)
            || !candidate_value(
                target.input.protected_right,
                studio_fm_right,
                target.input.exact_fm_right,
                right))
            return false;
        if (left < static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())
            || left > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())
            || right < static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())
            || right > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()))
            return false;

        target.final_left = static_cast<std::int32_t>(left);
        target.final_right = static_cast<std::int32_t>(right);
        target.status = state::final_studio;
        --waiting_;
        return true;
    }

    // Runtime evidence failure is allowed to reduce quality, never identity.
    // Convert every unresolved FM frame back to its protected-reference value;
    // already finalized Studio frames remain valid historical output.
    void fail_closed_reference() noexcept {
        if (!valid_)
            return;
        for (std::size_t i = 0; i < count_; ++i) {
            entry& current = entries_[(head_ + i) % Capacity];
            if (current.status != state::waiting_studio)
                continue;
            current.final_left = current.input.protected_left;
            current.final_right = current.input.protected_right;
            current.status = state::final_reference;
        }
        waiting_ = 0;
    }

    // At a true EOF, every unresolved frame must be precisely the impossible
    // symmetric-FIR tail reported by the source observer. The count check keeps
    // an ordinary block-boundary underrun from being mislabeled as EOF fallback.
    bool finish_reference_tail(std::size_t expected_tail_frames) noexcept {
        if (!valid_ || waiting_ != expected_tail_frames)
            return false;
        fail_closed_reference();
        return true;
    }

    bool pop_final(studio_transport_output_frame& out) noexcept {
        if (!valid_ || count_ == 0)
            return false;
        entry& current = entries_[head_];
        if (current.status == state::waiting_studio)
            return false;

        out.destination_ordinal = current.input.destination_ordinal;
        out.left = current.final_left;
        out.right = current.final_right;
        out.used_studio_fm = current.status == state::final_studio;
        head_ = (head_ + 1u) % Capacity;
        --count_;
        return true;
    }

private:
    static bool candidate_value(
        std::int32_t protected_value,
        std::int64_t studio_fm,
        std::int64_t exact_fm,
        std::int64_t& out) noexcept {
        const std::int64_t protected64 = static_cast<std::int64_t>(protected_value);
        if (studio_fm > 0
            && protected64 > std::numeric_limits<std::int64_t>::max() - studio_fm)
            return false;
        if (studio_fm < 0
            && protected64 < std::numeric_limits<std::int64_t>::min() - studio_fm)
            return false;
        const std::int64_t added = protected64 + studio_fm;
        if (exact_fm > 0
            && added < std::numeric_limits<std::int64_t>::min() + exact_fm)
            return false;
        if (exact_fm < 0
            && added > std::numeric_limits<std::int64_t>::max() + exact_fm)
            return false;
        out = added - exact_fm;
        return true;
    }

    std::array<entry, Capacity> entries_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    std::size_t waiting_ = 0;
    bool have_next_ordinal_ = false;
    std::uint64_t next_destination_ordinal_ = 0;
    bool valid_ = true;
};

} // namespace foobar_vgm::source_audio
