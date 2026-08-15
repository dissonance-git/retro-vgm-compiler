#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace vgmtooling::model {

// Deep auditory evidence is allowed to inform continuity and re-identification,
// but none of these cues can silently become source truth. The cue vocabulary
// follows the current deepSTRF/libaural research boundary while remaining small
// enough for a future compressed realtime implementation.
enum class realtime_auditory_continuity_cue : std::uint32_t {
    none = 0,
    source_semantic = 1u << 0,
    phase_trajectory = 1u << 1,
    level_trajectory = 1u << 2,
    dropout = 1u << 3,
    spectral_relation = 1u << 4,
    onset_relation = 1u << 5,
    persistent_part = 1u << 6,
};

constexpr std::uint32_t auditory_continuity_cue_mask(
    realtime_auditory_continuity_cue cue) noexcept
{
    return static_cast<std::uint32_t>(cue);
}

struct realtime_auditory_evidence_value {
    bool available = false;
    float score = 0.0f;      // [0, 1], cue-specific support
    float confidence = 0.0f; // [0, 1], reliability of that support
};

struct realtime_auditory_continuity_evidence {
    realtime_auditory_evidence_value phase_trajectory{};
    realtime_auditory_evidence_value level_trajectory{};
    realtime_auditory_evidence_value dropout_continuity{};
    realtime_auditory_evidence_value spectral_relation{};
    realtime_auditory_evidence_value onset_relation{};

    // Optional exact/stronger external ownership evidence. This is intentionally
    // separate from acoustic continuity because acoustically identical hidden
    // source swaps are not observable from waveform evidence alone.
    bool source_identity_supported = false;
    float source_identity_confidence = 0.0f;

    bool persistent_part_supported = false;
    std::uint64_t persistent_part_id = 0;
    float persistent_part_confidence = 0.0f;
};

// Uncertainty is split by obligation rather than collapsed into one confidence
// scalar. A representation can preserve continuity while being poor at precise
// pitch, or preserve a stale object template while lacking permission to update
// that template under masking.
struct realtime_auditory_state_confidence {
    float sensory = 0.0f;
    float object = 0.0f;
    float continuity = 0.0f;
    float precision = 0.0f;
    float reidentification = 0.0f;
    float plasticity = 0.0f;
};

enum class realtime_identity_hypothesis_domain : std::uint8_t {
    source_episode = 0,
    persistent_part,
    auditory_object,
};

struct realtime_identity_hypothesis {
    realtime_identity_hypothesis_domain domain =
        realtime_identity_hypothesis_domain::auditory_object;
    std::uint64_t id = 0;
    std::uint64_t generation = 0;
    float support = 0.0f;
    float confidence = 0.0f;
    std::uint32_t cues = 0;
};

constexpr float clamp_auditory_unit(float value) noexcept
{
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

constexpr realtime_auditory_evidence_value normalized_auditory_evidence(
    bool available,
    float score,
    float confidence) noexcept
{
    return {
        available,
        available ? clamp_auditory_unit(score) : 0.0f,
        available ? clamp_auditory_unit(confidence) : 0.0f,
    };
}

// Fixed-capacity ambiguity-preserving re-identification set. There is
// deliberately no best() or winner() helper: a caller must make any collapse
// explicit and policy-specific instead of turning an ambiguous return into a
// fictional certainty.
template <std::size_t Capacity = 8>
class realtime_identity_hypothesis_set {
public:
    static_assert(Capacity > 0, "identity hypothesis set needs capacity");

    void clear() noexcept { count_ = 0; }
    std::size_t size() const noexcept { return count_; }
    bool empty() const noexcept { return count_ == 0; }

    const realtime_identity_hypothesis& operator[](std::size_t index) const noexcept {
        return hypotheses_[index];
    }

    bool add(realtime_identity_hypothesis hypothesis) noexcept {
        hypothesis.support = clamp_auditory_unit(hypothesis.support);
        hypothesis.confidence = clamp_auditory_unit(hypothesis.confidence);

        for (std::size_t index = 0; index < count_; ++index) {
            realtime_identity_hypothesis& existing = hypotheses_[index];
            if (existing.domain != hypothesis.domain ||
                existing.id != hypothesis.id ||
                existing.generation != hypothesis.generation)
                continue;

            // Merge repeated support for the same candidate without comparing
            // it against competing identities. Competing candidates remain.
            if (hypothesis.confidence > existing.confidence) {
                existing.support = hypothesis.support;
                existing.confidence = hypothesis.confidence;
            }
            existing.cues |= hypothesis.cues;
            return true;
        }

        if (count_ >= Capacity)
            return false;
        hypotheses_[count_++] = hypothesis;
        return true;
    }

private:
    std::array<realtime_identity_hypothesis, Capacity> hypotheses_{};
    std::size_t count_ = 0;
};

// Plasticity is intentionally stricter than continuity. Weak or ambiguous
// continuity may be enough to keep a current object hypothesis alive, while
// durable template/role revision should freeze until ownership and evidence
// quality recover.
constexpr bool may_update_durable_auditory_memory(
    const realtime_auditory_state_confidence& confidence,
    float minimum_continuity = 0.70f,
    float minimum_plasticity = 0.70f) noexcept
{
    return clamp_auditory_unit(confidence.continuity) >= minimum_continuity &&
        clamp_auditory_unit(confidence.plasticity) >= minimum_plasticity;
}

} // namespace vgmtooling::model
