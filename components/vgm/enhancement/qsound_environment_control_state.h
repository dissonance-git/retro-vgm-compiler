#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

constexpr std::uint8_t qsound_environment_global_channel = 0xffu;

enum class qsound_environment_control_kind : std::uint8_t {
    echo_feedback = 0,
    echo_end_position = 1,
    wet_filter_table = 2,
    dry_filter_table = 3,
    wet_delay = 4,
    dry_delay = 5,
    delay_update = 6,
    next_state = 7,
    wet_volume = 8,
    dry_volume = 9,
};

struct qsound_environment_control_write {
    qsound_environment_control_kind kind = qsound_environment_control_kind::echo_feedback;
    std::uint8_t channel = qsound_environment_global_channel;
    std::uint16_t raw_value = 0;
};

struct qsound_observed_word {
    bool known = false;
    std::uint16_t raw_value = 0;
};

constexpr bool qsound_decode_environment_control_write(
    std::uint8_t address,
    std::uint16_t value,
    qsound_environment_control_write& out) noexcept
{
    out.raw_value = value;
    out.channel = qsound_environment_global_channel;

    switch (address) {
    case 0x93u:
        out.kind = qsound_environment_control_kind::echo_feedback;
        return true;
    case 0xd9u:
        out.kind = qsound_environment_control_kind::echo_end_position;
        return true;
    case 0xe2u:
        out.kind = qsound_environment_control_kind::delay_update;
        return true;
    case 0xe3u:
        out.kind = qsound_environment_control_kind::next_state;
        return true;

    case 0xdau:
        out.kind = qsound_environment_control_kind::wet_filter_table;
        out.channel = 0;
        return true;
    case 0xdcu:
        out.kind = qsound_environment_control_kind::wet_filter_table;
        out.channel = 1;
        return true;
    case 0xdbu:
        out.kind = qsound_environment_control_kind::dry_filter_table;
        out.channel = 0;
        return true;
    case 0xddu:
        out.kind = qsound_environment_control_kind::dry_filter_table;
        out.channel = 1;
        return true;

    case 0xdeu:
        out.kind = qsound_environment_control_kind::wet_delay;
        out.channel = 0;
        return true;
    case 0xe0u:
        out.kind = qsound_environment_control_kind::wet_delay;
        out.channel = 1;
        return true;
    case 0xdfu:
        out.kind = qsound_environment_control_kind::dry_delay;
        out.channel = 0;
        return true;
    case 0xe1u:
        out.kind = qsound_environment_control_kind::dry_delay;
        out.channel = 1;
        return true;

    case 0xe4u:
        out.kind = qsound_environment_control_kind::wet_volume;
        out.channel = 0;
        return true;
    case 0xe6u:
        out.kind = qsound_environment_control_kind::wet_volume;
        out.channel = 1;
        return true;
    case 0xe5u:
        out.kind = qsound_environment_control_kind::dry_volume;
        out.channel = 0;
        return true;
    case 0xe7u:
        out.kind = qsound_environment_control_kind::dry_volume;
        out.channel = 1;
        return true;
    default:
        return false;
    }
}

// Command-layer shadow only. It records environmental control words actually
// observed in the VGM stream. It deliberately does not manufacture the QSound
// core's mode-specific initialization values, FIR taps, echo history, delay
// cursors or state-machine progress. Those remain internal renderer state until
// a separate replay earns them.
class qsound_environment_control_state {
public:
    qsound_environment_control_state() noexcept { reset(); }

    void reset() noexcept {
        echo_feedback_ = {};
        echo_end_position_ = {};
        wet_filter_table_.fill({});
        dry_filter_table_.fill({});
        wet_delay_.fill({});
        dry_delay_.fill({});
        delay_update_ = {};
        next_state_ = {};
        wet_volume_.fill({});
        dry_volume_.fill({});
    }

    bool apply(std::uint8_t address, std::uint16_t value) noexcept {
        qsound_environment_control_write write;
        if (!qsound_decode_environment_control_write(address, value, write))
            return false;
        return apply(write);
    }

    bool apply(const qsound_environment_control_write& write) noexcept {
        qsound_observed_word* target = nullptr;

        switch (write.kind) {
        case qsound_environment_control_kind::echo_feedback:
            if (write.channel != qsound_environment_global_channel)
                return false;
            target = &echo_feedback_;
            break;
        case qsound_environment_control_kind::echo_end_position:
            if (write.channel != qsound_environment_global_channel)
                return false;
            target = &echo_end_position_;
            break;
        case qsound_environment_control_kind::delay_update:
            if (write.channel != qsound_environment_global_channel)
                return false;
            target = &delay_update_;
            break;
        case qsound_environment_control_kind::next_state:
            if (write.channel != qsound_environment_global_channel)
                return false;
            target = &next_state_;
            break;

        case qsound_environment_control_kind::wet_filter_table:
            if (write.channel >= wet_filter_table_.size())
                return false;
            target = &wet_filter_table_[write.channel];
            break;
        case qsound_environment_control_kind::dry_filter_table:
            if (write.channel >= dry_filter_table_.size())
                return false;
            target = &dry_filter_table_[write.channel];
            break;
        case qsound_environment_control_kind::wet_delay:
            if (write.channel >= wet_delay_.size())
                return false;
            target = &wet_delay_[write.channel];
            break;
        case qsound_environment_control_kind::dry_delay:
            if (write.channel >= dry_delay_.size())
                return false;
            target = &dry_delay_[write.channel];
            break;
        case qsound_environment_control_kind::wet_volume:
            if (write.channel >= wet_volume_.size())
                return false;
            target = &wet_volume_[write.channel];
            break;
        case qsound_environment_control_kind::dry_volume:
            if (write.channel >= dry_volume_.size())
                return false;
            target = &dry_volume_[write.channel];
            break;
        }

        if (target == nullptr)
            return false;
        target->known = true;
        target->raw_value = write.raw_value;
        return true;
    }

    const qsound_observed_word& echo_feedback() const noexcept { return echo_feedback_; }
    const qsound_observed_word& echo_end_position() const noexcept { return echo_end_position_; }
    const qsound_observed_word& delay_update() const noexcept { return delay_update_; }
    const qsound_observed_word& next_state() const noexcept { return next_state_; }

    const qsound_observed_word& wet_filter_table(std::size_t channel) const noexcept {
        return channel < wet_filter_table_.size() ? wet_filter_table_[channel] : unknown_;
    }
    const qsound_observed_word& dry_filter_table(std::size_t channel) const noexcept {
        return channel < dry_filter_table_.size() ? dry_filter_table_[channel] : unknown_;
    }
    const qsound_observed_word& wet_delay(std::size_t channel) const noexcept {
        return channel < wet_delay_.size() ? wet_delay_[channel] : unknown_;
    }
    const qsound_observed_word& dry_delay(std::size_t channel) const noexcept {
        return channel < dry_delay_.size() ? dry_delay_[channel] : unknown_;
    }
    const qsound_observed_word& wet_volume(std::size_t channel) const noexcept {
        return channel < wet_volume_.size() ? wet_volume_[channel] : unknown_;
    }
    const qsound_observed_word& dry_volume(std::size_t channel) const noexcept {
        return channel < dry_volume_.size() ? dry_volume_[channel] : unknown_;
    }

private:
    qsound_observed_word echo_feedback_{};
    qsound_observed_word echo_end_position_{};
    std::array<qsound_observed_word, 2> wet_filter_table_{};
    std::array<qsound_observed_word, 2> dry_filter_table_{};
    std::array<qsound_observed_word, 2> wet_delay_{};
    std::array<qsound_observed_word, 2> dry_delay_{};
    qsound_observed_word delay_update_{};
    qsound_observed_word next_state_{};
    std::array<qsound_observed_word, 2> wet_volume_{};
    std::array<qsound_observed_word, 2> dry_volume_{};
    qsound_observed_word unknown_{};
};

} // namespace gameaudio::vgm
