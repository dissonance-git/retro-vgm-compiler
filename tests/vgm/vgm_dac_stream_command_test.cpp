#include "vgm_dac_stream_command.h"

#include <array>
#include <cassert>
#include <cstdint>

using namespace gameaudio::vgm;

template<std::size_t N>
static command_event make_event(
    const std::uint8_t command,
    const std::array<std::uint8_t, N>& payload) {
    return command_event{
        command_event_kind::command,
        123,
        456,
        command,
        payload.data(),
        static_cast<std::uint32_t>(payload.size()),
    };
}

int main() {
    constexpr std::array setup_payload{
        std::uint8_t{0x03}, std::uint8_t{0x02},
        std::uint8_t{0x00}, std::uint8_t{0x2A}};

    // DAC Stream Control arrived in VGM 1.60. Older files must not acquire
    // 0x90-series semantics retroactively.
    assert(!decode_dac_stream_command(
        vgm_version_1_51,
        make_event(0x90, setup_payload)).has_value());

    const auto setup = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x90, setup_payload));
    assert(setup.has_value());
    assert(setup->kind == dac_stream_command_kind::setup);
    assert(setup->stream_id == 0x03);
    assert(!setup->reserved_stream_id);
    assert(setup->chip_type == 0x02); // YM2612 in VGM clock/header order.
    assert(setup->chip_instance == 0);
    assert(setup->destination_port == 0x00);
    assert(setup->destination_command == 0x2A);

    // Setup bit 7 selects the second chip independently of chip family.
    constexpr std::array second_chip_payload{
        std::uint8_t{0x04}, std::uint8_t{0x85},
        std::uint8_t{0xFF}, std::uint8_t{0x07}};
    const auto second = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x90, second_chip_payload));
    assert(second.has_value());
    assert(second->chip_type == 0x05);
    assert(second->chip_instance == 1);
    assert(second->destination_port == 0xFF);
    assert(second->destination_command == 0x07);

    constexpr std::array data_payload{
        std::uint8_t{0x03}, std::uint8_t{0x11},
        std::uint8_t{0x02}, std::uint8_t{0x01}};
    const auto data = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x91, data_payload));
    assert(data.has_value());
    assert(data->kind == dac_stream_command_kind::set_data);
    assert(data->data_bank_id == 0x11);
    assert(data->step_size == 2);
    assert(data->step_base == 1);

    constexpr std::array frequency_payload{
        std::uint8_t{0x03}, std::uint8_t{0x44},
        std::uint8_t{0xAC}, std::uint8_t{0x00}, std::uint8_t{0x00}};
    const auto frequency = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x92, frequency_payload));
    assert(frequency.has_value());
    assert(frequency->kind == dac_stream_command_kind::set_frequency);
    assert(frequency->frequency_hz == 44'100u);

    constexpr std::array start_payload{
        std::uint8_t{0x03},
        std::uint8_t{0x78}, std::uint8_t{0x56},
        std::uint8_t{0x34}, std::uint8_t{0x12},
        std::uint8_t{0x92}, // milliseconds + reverse + loop
        std::uint8_t{0x04}, std::uint8_t{0x03},
        std::uint8_t{0x02}, std::uint8_t{0x01},
    };
    const auto start = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x93, start_payload));
    assert(start.has_value());
    assert(start->kind == dac_stream_command_kind::start);
    assert(start->start_offset == 0x12345678u);
    assert(start->play_mode == 0x92);
    assert(start->length == 0x01020304u);
    assert(start->loop);
    assert(start->reverse);

    // 0xFFFFFFFF is the spec-defined sentinel for preserving current position.
    constexpr std::array keep_position_payload{
        std::uint8_t{0x03},
        std::uint8_t{0xFF}, std::uint8_t{0xFF},
        std::uint8_t{0xFF}, std::uint8_t{0xFF},
        std::uint8_t{0x00},
        std::uint8_t{0x00}, std::uint8_t{0x00},
        std::uint8_t{0x00}, std::uint8_t{0x00},
    };
    const auto keep_position = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x93, keep_position_payload));
    assert(keep_position.has_value());
    assert(keep_position->start_offset == 0xFFFFFFFFu);

    constexpr std::array stop_one_payload{std::uint8_t{0x03}};
    const auto stop_one = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x94, stop_one_payload));
    assert(stop_one.has_value());
    assert(stop_one->kind == dac_stream_command_kind::stop);
    assert(!stop_one->stop_all);

    constexpr std::array stop_all_payload{std::uint8_t{0xFF}};
    const auto stop_all = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x94, stop_all_payload));
    assert(stop_all.has_value());
    assert(stop_all->stop_all);
    assert(!stop_all->reserved_stream_id);

    constexpr std::array fast_payload{
        std::uint8_t{0x03}, std::uint8_t{0x34},
        std::uint8_t{0x12}, std::uint8_t{0x11}};
    const auto fast = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x95, fast_payload));
    assert(fast.has_value());
    assert(fast->kind == dac_stream_command_kind::fast_start);
    assert(fast->block_id == 0x1234);
    assert(fast->fast_flags == 0x11);
    assert(fast->loop);
    assert(fast->reverse);

    // 0xFF is reserved for all stream commands except stop-all.
    constexpr std::array reserved_payload{
        std::uint8_t{0xFF}, std::uint8_t{0x02},
        std::uint8_t{0x00}, std::uint8_t{0x2A}};
    const auto reserved = decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x90, reserved_payload));
    assert(reserved.has_value());
    assert(reserved->reserved_stream_id);

    // Fail closed on unrelated commands, wrong payload sizes, and reset events.
    assert(!decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x8F, setup_payload)).has_value());

    constexpr std::array short_setup{
        std::uint8_t{0x03}, std::uint8_t{0x02}, std::uint8_t{0x00}};
    assert(!decode_dac_stream_command(
        vgm_version_1_60,
        make_event(0x90, short_setup)).has_value());

    auto reset_event = make_event(0x90, setup_payload);
    reset_event.kind = command_event_kind::reset;
    assert(!decode_dac_stream_command(vgm_version_1_60, reset_event).has_value());

    return 0;
}
