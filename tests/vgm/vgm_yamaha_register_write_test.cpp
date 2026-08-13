#include "components/vgm/enhancement/vgm_yamaha_register_write.h"

#include <array>
#include <cassert>
#include <cstdint>

using namespace gameaudio::vgm;

static command_trace_record make_write(
    const std::uint8_t command,
    const std::uint8_t address = 0x28,
    const std::uint8_t data = 0xF0) {
    command_trace_record record;
    record.kind = command_event_kind::command;
    record.command = command;
    record.payload_size = 2;
    record.payload_prefix = {address, data};
    record.payload_prefix_size = 2;
    record.payload_truncated = false;
    return record;
}

int main() {
    struct expected_write {
        std::uint8_t command;
        yamaha_register_target target;
        std::uint8_t instance;
        std::uint8_t port;
    };

    constexpr std::array primary = {
        expected_write{0x51, yamaha_register_target::ym2413, 0, 0},
        expected_write{0x52, yamaha_register_target::ym2612, 0, 0},
        expected_write{0x53, yamaha_register_target::ym2612, 0, 1},
        expected_write{0x54, yamaha_register_target::ym2151, 0, 0},
        expected_write{0x55, yamaha_register_target::ym2203, 0, 0},
        expected_write{0x56, yamaha_register_target::ym2608, 0, 0},
        expected_write{0x57, yamaha_register_target::ym2608, 0, 1},
        expected_write{0x58, yamaha_register_target::ym2610, 0, 0},
        expected_write{0x59, yamaha_register_target::ym2610, 0, 1},
        expected_write{0x5A, yamaha_register_target::ym3812, 0, 0},
        expected_write{0x5B, yamaha_register_target::ym3526, 0, 0},
        expected_write{0x5C, yamaha_register_target::y8950, 0, 0},
        expected_write{0x5D, yamaha_register_target::ymz280b, 0, 0},
        expected_write{0x5E, yamaha_register_target::ymf262, 0, 0},
        expected_write{0x5F, yamaha_register_target::ymf262, 0, 1},
    };

    for (const auto& expected : primary) {
        const auto decoded = decode_yamaha_register_write(make_write(expected.command));
        assert(decoded.has_value());
        assert(decoded->target == expected.target);
        assert(decoded->instance == expected.instance);
        assert(decoded->port == expected.port);
        assert(decoded->address == 0x28);
        assert(decoded->data == 0xF0);
        assert(decoded->source_command == expected.command);

        const std::uint8_t second_command = static_cast<std::uint8_t>(expected.command + 0x50);
        const auto second = decode_yamaha_register_write(make_write(second_command, 0xB4, 0xC0));
        assert(second.has_value());
        assert(second->target == expected.target);
        assert(second->instance == 1);
        assert(second->port == expected.port);
        assert(second->address == 0xB4);
        assert(second->data == 0xC0);
        assert(second->source_command == second_command);
    }

    // 0xA0 is AY8910, not a mirrored Yamaha-family write.
    assert(!decode_yamaha_register_write(make_write(0xA0)).has_value());

    // Waits and unrelated commands remain opaque.
    assert(!decode_yamaha_register_write(make_write(0x61)).has_value());

    // Incomplete source evidence must fail closed rather than inventing a write.
    auto truncated = make_write(0x56);
    truncated.payload_prefix_size = 1;
    truncated.payload_truncated = true;
    assert(!decode_yamaha_register_write(truncated).has_value());

    auto wrong_size = make_write(0x56);
    wrong_size.payload_size = 1;
    wrong_size.payload_prefix_size = 1;
    assert(!decode_yamaha_register_write(wrong_size).has_value());

    auto reset = make_write(0x56);
    reset.kind = command_event_kind::reset;
    assert(!decode_yamaha_register_write(reset).has_value());

    return 0;
}
