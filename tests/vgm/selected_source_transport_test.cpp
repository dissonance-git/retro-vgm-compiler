#include "components/vgm/enhancement/selected_source_transport.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

using namespace gameaudio::vgm;

int main() {
    using frame_type = selected_source_frame<3>;

    selected_source_queue<3, 8> queue;
    queue.reset(10u);

    frame_type first{};
    first.ordinal = 10u;
    first.source[0] = {1.0, -1.0, true, true};
    first.source[1] = {0.0, 0.0, true, true};
    frame_type second{};
    second.ordinal = 11u;
    second.source[0] = {2.0, -2.0, true, true};
    second.source[1] = {0.0, 0.0, true, true};

    assert(queue.push_reference(first));
    assert(queue.push_reference(second));
    assert(queue.replace_source(11u, 0u, 3.0, -3.0, true));

    selected_source_block_storage<3, 4> block;
    assert(block.consume(queue, 10u, 2u));
    assert(block.valid());
    assert(block.source_present(0u));
    assert(!block.source_present(1u));
    assert(block.sources()[0].left[1] == 3.0f);

    // All-silent delivery consumes the ordinal but is not source-clock failure.
    queue.reset(20u);
    frame_type silent{};
    silent.ordinal = 20u;
    silent.source[0] = {0.0, 0.0, true, true};
    frame_type audible{};
    audible.ordinal = 21u;
    audible.source[0] = {4.0, 4.0, true, true};
    assert(queue.push_reference(silent));
    assert(queue.push_reference(audible));
    assert(!block.consume(queue, 20u, 1u));
    assert(block.last_error() == selected_source_block_error::no_sources);
    assert(queue.valid() && queue.size() == 1u);
    assert(block.consume(queue, 21u, 1u));

    // A source appearing inside one delivered block changes topology and makes
    // the source/object interpretation unsafe for that transport generation.
    queue.reset(30u);
    frame_type topology_a{};
    topology_a.ordinal = 30u;
    topology_a.source[0] = {1.0, 1.0, true, true};
    frame_type topology_b{};
    topology_b.ordinal = 31u;
    topology_b.source[1] = {1.0, 1.0, true, true};
    assert(queue.push_reference(topology_a));
    assert(queue.push_reference(topology_b));
    assert(!block.consume(queue, 30u, 2u));
    assert(block.last_error() == selected_source_block_error::topology_changed);
    assert(!queue.valid());

    // Inexact producer provenance cannot replace the queued exact lane.
    queue.reset(40u);
    frame_type exact{};
    exact.ordinal = 40u;
    exact.source[0] = {1.0, 1.0, true, true};
    assert(queue.push_reference(exact));
    assert(!queue.replace_source(40u, 0u, 8.0, 8.0, false));
    assert(queue.valid());
    assert(block.consume(queue, 40u, 1u));
    assert(block.sources()[0].left[0] == 1.0f);

    return 0;
}
