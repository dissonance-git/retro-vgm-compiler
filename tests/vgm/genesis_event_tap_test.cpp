#include "../../components/vgm/enhancement/genesis_state.h"

#include <cstdint>

using gameaudio::vgm::command_event;
using gameaudio::vgm::command_event_kind;
using gameaudio::vgm::genesis_state;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {
struct tap_state {
    std::uint32_t calls = 0;
    std::uint32_t resets = 0;
    std::uint8_t last_command = 0;
};

void tap(void* user, const command_event& event) noexcept {
    auto* state = static_cast<tap_state*>(user);
    ++state->calls;
    if (event.kind == command_event_kind::reset)
        ++state->resets;
    state->last_command = event.command;
}
}

int main() {
    genesis_state state;
    tap_state observed;
    state.set_event_tap(&tap, &observed);

    const std::uint8_t psg = 0x90;
    state.observe(command_event{command_event_kind::command, 10, 20, 0x50, &psg, 1});
    CHECK(observed.calls == 1);
    CHECK(observed.last_command == 0x50);
    CHECK(state.psg_writes() == 1);

    state.observe(command_event{command_event_kind::reset});
    CHECK(observed.calls == 2);
    CHECK(observed.resets == 1);
    CHECK(state.psg_writes() == 0);

    // A reset must clear musical state without severing the observer used by
    // the realtime shadow renderer.
    const std::uint8_t ym[] = {0x2B, 0x80};
    state.observe(command_event{command_event_kind::command, 30, 40, 0x52, ym, 2});
    CHECK(observed.calls == 3);
    CHECK(observed.last_command == 0x52);
    CHECK(state.ym2612().dac_enabled);

    state.set_event_tap(nullptr, nullptr);
    state.observe(command_event{command_event_kind::command, 50, 60, 0x50, &psg, 1});
    CHECK(observed.calls == 3);
    CHECK(state.psg_writes() == 1);

    return 0;
}
