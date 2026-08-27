#include "components/vgm/enhancement/selected_source_transport.h"
#include "components/vgm/enhancement/ym2151_enhanced_recomposition.h"

#include <cassert>
#include <cstddef>

using namespace gameaudio::vgm;

int main() {
    constexpr std::size_t fm1 =
        static_cast<std::size_t>(ym2151_recomposition_source::fm1);
    constexpr std::size_t fm8 =
        static_cast<std::size_t>(ym2151_recomposition_source::fm8);

    selected_source_queue<ym2151_recomposition_source_count, 8> queue;
    queue.reset(100u);

    selected_source_frame<ym2151_recomposition_source_count> first{};
    first.ordinal = 100u;
    first.source[fm1] = {1.0, -1.0, true, true};
    first.source[fm8] = {0.0, 0.0, true, true};
    selected_source_frame<ym2151_recomposition_source_count> second{};
    second.ordinal = 101u;
    second.source[fm1] = {2.0, -2.0, true, true};
    second.source[fm8] = {0.0, 0.0, true, true};

    assert(queue.push_reference(first));
    assert(queue.push_reference(second));
    assert(queue.replace_source(101u, fm1, 8.0, -8.0, true));

    selected_source_block_storage<ym2151_recomposition_source_count, 4> block;
    assert(block.consume(queue, 100u, 2u));
    assert(block.valid());
    assert(block.source_present(fm1));
    assert(!block.source_present(fm8));
    assert(block.sources()[fm1].left[1] == 8.0f);
    assert(block.sources()[fm1].right[1] == -8.0f);

    // A complete silent OPM channel can disappear from the presentation source
    // set, but a source appearing/disappearing inside one delivered block is an
    // unsafe topology change and fails the transport generation closed.
    queue.reset(200u);
    first.ordinal = 200u;
    second.ordinal = 201u;
    first.source[fm8] = {};
    second.source[fm8] = {1.0, 1.0, true, true};
    assert(queue.push_reference(first));
    assert(queue.push_reference(second));
    assert(!block.consume(queue, 200u, 2u));
    assert(block.last_error() == selected_source_block_error::topology_changed);
    assert(!queue.valid());

    // Presentation consumes already-selected lanes. Rejecting an inexact
    // enhanced candidate therefore leaves the exact reference OPM lane queued.
    queue.reset(300u);
    selected_source_frame<ym2151_recomposition_source_count> reference{};
    reference.ordinal = 300u;
    reference.source[fm1] = {2.0, -2.0, true, true};
    assert(queue.push_reference(reference));
    assert(!queue.replace_source(300u, fm1, 9.0, -9.0, false));
    assert(queue.valid());
    assert(block.consume(queue, 300u, 1u));
    assert(block.sources()[fm1].left[0] == 2.0f);
    assert(block.sources()[fm1].right[0] == -2.0f);

    return 0;
}
