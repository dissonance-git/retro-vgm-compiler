#pragma once

#include "studio_source_timeline.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace foobar_vgm::source_audio {

// Whole-frame synchronization queue for a future-dependent source replacement.
//
// Payload is intentionally opaque to this primitive. In the foobar integration
// it represents the protected host frame plus every exact source-family value
// that must remain phase-locked to it. Only the FM reconstruction has lookahead;
// DAC, PSG, untouched chips, and metadata simply travel with the same ordinal.
//
// A frame can leave the queue only after its native source support has been
// verified. FIR latency is therefore an explicit scheduling fact instead of an
// implicit FM-only delay.
template <typename Payload, std::size_t Capacity>
class studio_alignment_queue {
    static_assert(Capacity > 0, "alignment queue capacity must be non-zero.");
    static_assert(std::is_copy_assignable<Payload>::value,
        "alignment payload must be copy assignable.");

public:
    struct entry {
        std::uint64_t destination_ordinal = 0;
        studio_source_phase_position source_position{};
        Payload payload{};
    };

    void reset() noexcept {
        head_ = 0;
        count_ = 0;
        invalid_ = false;
        have_next_ordinal_ = false;
        next_destination_ordinal_ = 0;
    }

    [[nodiscard]] bool valid() const noexcept { return !invalid_; }
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    [[nodiscard]] bool full() const noexcept { return count_ == Capacity; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }

    bool push(
        std::uint64_t destination_ordinal,
        studio_source_phase_position source_position,
        const Payload& payload) noexcept {
        if (invalid_ || full() || !source_position.valid) {
            invalid_ = true;
            return false;
        }

        if (have_next_ordinal_) {
            if (destination_ordinal != next_destination_ordinal_) {
                invalid_ = true;
                return false;
            }
        } else {
            have_next_ordinal_ = true;
        }

        if (destination_ordinal == std::numeric_limits<std::uint64_t>::max()) {
            invalid_ = true;
            return false;
        }

        entry& dst = entries_[(head_ + count_) % Capacity];
        dst.destination_ordinal = destination_ordinal;
        dst.source_position = source_position;
        dst.payload = payload;
        ++count_;
        next_destination_ordinal_ = destination_ordinal + 1u;
        return true;
    }

    [[nodiscard]] const entry* front() const noexcept {
        if (invalid_ || count_ == 0)
            return nullptr;
        return &entries_[head_];
    }

    template <typename NativeStream>
    [[nodiscard]] bool front_ready(const NativeStream& stream) const noexcept {
        const entry* current = front();
        if (current == nullptr || !stream.valid())
            return false;
        return stream.contains(plan_studio_source_window(current->source_position));
    }

    template <typename NativeStream>
    bool pop_ready(const NativeStream& stream, entry& out) noexcept {
        return pop_ready_when(front_ready(stream), out);
    }

    // Some proven boundaries, such as a fresh OPN2's silent negative-time FM
    // prefix, require a readiness predicate richer than NativeStream::contains.
    // The caller still has to prove the complete FIR window before dequeueing;
    // false never mutates the queue.
    bool pop_ready_when(bool ready, entry& out) noexcept {
        const entry* current = front();
        if (!ready || current == nullptr)
            return false;
        out = *current;
        head_ = (head_ + 1u) % Capacity;
        --count_;
        return true;
    }

    // End-of-stream fallback. Once the authoritative producer has ended, the
    // final post_roll frames can never gain future FIR support. The integration
    // may release their protected-reference payloads explicitly, but only via
    // this distinct operation so tail fallback can never masquerade as enhanced.
    bool pop_reference_tail(entry& out) noexcept {
        const entry* current = front();
        if (current == nullptr)
            return false;
        out = *current;
        head_ = (head_ + 1u) % Capacity;
        --count_;
        return true;
    }

private:
    std::array<entry, Capacity> entries_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    bool invalid_ = false;
    bool have_next_ordinal_ = false;
    std::uint64_t next_destination_ordinal_ = 0;
};

} // namespace foobar_vgm::source_audio
