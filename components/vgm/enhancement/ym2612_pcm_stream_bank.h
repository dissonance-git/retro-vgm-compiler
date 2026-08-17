#pragma once
#include "ym2612_pcm_stream.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

class ym2612_pcm_stream_bank {
public:
    static constexpr std::size_t stream_count = 256;

    explicit ym2612_pcm_stream_bank(double rate = 48000.0) noexcept {
        configure_output_rate(rate);
        reset();
    }

    void configure_output_rate(double rate) noexcept {
        rate_ = rate > 0.0 ? rate : 48000.0;
        for (auto& stream : streams_) stream.configure_output_rate(rate_);
    }

    void reset() noexcept {
        target_.fill(false);
        for (auto& stream : streams_) {
            stream.configure_output_rate(rate_);
            stream.reset();
        }
    }

    void apply(const dac_stream_source_event& event) noexcept {
        const std::size_t id = event.stream_id;
        const bool target = event.chip_type == 0x02 && event.chip_id == 0
            && (event.chip_command & 0x00FFu) == 0x2Au;
        target_[id] = target;
        if (target) streams_[id].apply(event);
        else if (event.kind == dac_stream_source_event_kind::setup) streams_[id].reset();
    }

    bool render(float* output, std::size_t frames) noexcept {
        if (output) for (std::size_t i = 0; i < frames; ++i) output[i] = 0.0f;
        std::size_t count = 0, owner = 0;
        for (std::size_t id = 0; id < stream_count; ++id) {
            if (target_[id] && streams_[id].active() && streams_[id].valid()) {
                ++count; owner = id;
            }
        }
        if (count == 1) {
            streams_[owner].render(output, frames);
            return true;
        }
        for (std::size_t id = 0; id < stream_count; ++id)
            if (target_[id] && streams_[id].active() && streams_[id].valid()) streams_[id].advance(frames);
        return count == 0;
    }

    std::size_t active_target_count() const noexcept {
        std::size_t count = 0;
        for (std::size_t id = 0; id < stream_count; ++id)
            count += target_[id] && streams_[id].active() && streams_[id].valid() ? 1u : 0u;
        return count;
    }

private:
    std::array<ym2612_pcm_stream, stream_count> streams_{};
    std::array<bool, stream_count> target_{};
    double rate_ = 48000.0;
};

} // namespace gameaudio::vgm
