#include "../../components/vgm/enhancement/qsound_control_state.h"
#include "../../components/vgm/enhancement/qsound_environment_control_state.h"

#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    qsound_environment_control_write write;

    CHECK(qsound_decode_environment_control_write(0x93, 0xff00, write));
    CHECK(write.kind == qsound_environment_control_kind::echo_feedback);
    CHECK(write.channel == qsound_environment_global_channel);
    CHECK(write.raw_value == 0xff00u);

    CHECK(qsound_decode_environment_control_write(0xd9, 0x055a, write));
    CHECK(write.kind == qsound_environment_control_kind::echo_end_position);
    CHECK(write.channel == qsound_environment_global_channel);

    CHECK(qsound_decode_environment_control_write(0xda, 0x0db2, write));
    CHECK(write.kind == qsound_environment_control_kind::wet_filter_table);
    CHECK(write.channel == 0u);
    CHECK(qsound_decode_environment_control_write(0xdc, 0x0e11, write));
    CHECK(write.kind == qsound_environment_control_kind::wet_filter_table);
    CHECK(write.channel == 1u);

    CHECK(qsound_decode_environment_control_write(0xdb, 0x0f73, write));
    CHECK(write.kind == qsound_environment_control_kind::dry_filter_table);
    CHECK(write.channel == 0u);
    CHECK(qsound_decode_environment_control_write(0xdd, 0x0fa4, write));
    CHECK(write.kind == qsound_environment_control_kind::dry_filter_table);
    CHECK(write.channel == 1u);

    CHECK(qsound_decode_environment_control_write(0xde, 1, write));
    CHECK(write.kind == qsound_environment_control_kind::wet_delay);
    CHECK(write.channel == 0u);
    CHECK(qsound_decode_environment_control_write(0xe0, 2, write));
    CHECK(write.kind == qsound_environment_control_kind::wet_delay);
    CHECK(write.channel == 1u);
    CHECK(qsound_decode_environment_control_write(0xdf, 3, write));
    CHECK(write.kind == qsound_environment_control_kind::dry_delay);
    CHECK(write.channel == 0u);
    CHECK(qsound_decode_environment_control_write(0xe1, 4, write));
    CHECK(write.kind == qsound_environment_control_kind::dry_delay);
    CHECK(write.channel == 1u);

    CHECK(qsound_decode_environment_control_write(0xe2, 1, write));
    CHECK(write.kind == qsound_environment_control_kind::delay_update);
    CHECK(write.channel == qsound_environment_global_channel);
    CHECK(qsound_decode_environment_control_write(0xe3, 0x0314, write));
    CHECK(write.kind == qsound_environment_control_kind::next_state);
    CHECK(write.channel == qsound_environment_global_channel);

    CHECK(qsound_decode_environment_control_write(0xe4, 0x3fff, write));
    CHECK(write.kind == qsound_environment_control_kind::wet_volume);
    CHECK(write.channel == 0u);
    CHECK(qsound_decode_environment_control_write(0xe6, 0x3ffe, write));
    CHECK(write.kind == qsound_environment_control_kind::wet_volume);
    CHECK(write.channel == 1u);
    CHECK(qsound_decode_environment_control_write(0xe5, 0x2000, write));
    CHECK(write.kind == qsound_environment_control_kind::dry_volume);
    CHECK(write.channel == 0u);
    CHECK(qsound_decode_environment_control_write(0xe7, 0x1000, write));
    CHECK(write.kind == qsound_environment_control_kind::dry_volume);
    CHECK(write.channel == 1u);

    // Source and shared-environment control ownership must remain disjoint.
    qsound_source_control_write source_write;
    CHECK(!qsound_decode_source_control_write(0x93, 0x1234, source_write));
    CHECK(!qsound_decode_environment_control_write(0x80, 0x0120, write));
    CHECK(!qsound_decode_environment_control_write(0xba, 0x0010, write));
    CHECK(!qsound_decode_environment_control_write(0x94, 0x0010, write));

    qsound_environment_control_state state;
    CHECK(!state.echo_feedback().known);
    CHECK(!state.wet_delay(0).known);
    CHECK(!state.wet_delay(9).known);

    CHECK(state.apply(0x93, 0xff00));
    CHECK(state.echo_feedback().known);
    CHECK(state.echo_feedback().raw_value == 0xff00u);

    CHECK(state.apply(0xde, 7));
    CHECK(state.wet_delay(0).known);
    CHECK(state.wet_delay(0).raw_value == 7u);
    CHECK(!state.wet_delay(1).known);

    CHECK(state.apply(0xe7, 0x2222));
    CHECK(state.dry_volume(1).known);
    CHECK(state.dry_volume(1).raw_value == 0x2222u);

    qsound_environment_control_write malformed;
    malformed.kind = qsound_environment_control_kind::wet_delay;
    malformed.channel = qsound_environment_global_channel;
    malformed.raw_value = 8;
    CHECK(!state.apply(malformed));

    state.reset();
    CHECK(!state.echo_feedback().known);
    CHECK(!state.wet_delay(0).known);
    CHECK(!state.dry_volume(1).known);

    return 0;
}
