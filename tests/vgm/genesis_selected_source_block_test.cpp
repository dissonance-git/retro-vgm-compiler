#include "components/vgm/enhancement/genesis_selected_source_block.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {
using namespace gameaudio::vgm;

constexpr std::size_t fm1 =
    static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1);
constexpr std::size_t psg0 =
    static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0);

genesis_selected_source_frame reference_frame(
    std::uint64_t ordinal,
    double fm_left,
    double fm_right,
    double psg_left,
    double psg_right) {
    genesis_selected_source_frame frame{};
    frame.ordinal = ordinal;
    frame.source[fm1] = {fm_left, fm_right, true, true};
    frame.source[psg0] = {psg_left, psg_right, true, true};
    return frame;
}
}

int main() {
    using namespace gameaudio::vgm;

    genesis_selected_source_queue<8> queue;
    queue.reset(100u);
    assert(queue.push_reference(reference_frame(100u, 1.0, -2.0, 0.25, 0.5)));
    assert(queue.push_reference(reference_frame(101u, 3.0, -4.0, 0.75, 1.0)));

    genesis_selected_source_block_storage<4> block;
    assert(block.consume(queue, 100u, 2u));
    assert(block.valid());
    assert(block.first_ordinal() == 100u);
    assert(block.frame_count() == 2u);
    assert(block.source_present(fm1));
    assert(block.source_present(psg0));
    assert(block.sources()[fm1].exact);
    assert(block.sources()[fm1].left[0] == 1.0f);
    assert(block.sources()[fm1].right[1] == -4.0f);
    assert(block.sources()[psg0].left[1] == 0.75f);
    assert(queue.size() == 0u);

    // Quality replacement happens before delivery; the block therefore exposes
    // exactly the already-selected lane without knowing why it was selected.
    queue.reset(200u);
    assert(queue.push_reference(reference_frame(200u, 1.0, 1.0, 2.0, 2.0)));
    assert(queue.replace_source(200u, fm1, 9.0, 8.0));
    assert(block.consume(queue, 200u, 1u));
    assert(block.sources()[fm1].left[0] == 9.0f);
    assert(block.sources()[fm1].right[0] == 8.0f);

    // Exact topology is still checked, but a source which contributes exact
    // digital zero throughout the delivered block is not a Spatial object. It
    // cannot veto an active lane merely because no route write ever occurred.
    queue.reset(225u);
    assert(queue.push_reference(reference_frame(225u, 0.0, 0.0, 2.0, -2.0)));
    assert(queue.push_reference(reference_frame(226u, 0.0, 0.0, 3.0, -3.0)));
    assert(block.consume(queue, 225u, 2u));
    assert(block.valid());
    assert(!block.source_present(fm1));
    assert(!block.sources()[fm1].present());
    assert(block.source_present(psg0));
    assert(block.sources()[psg0].present());

    // An all-silent block is not source-clock corruption. It declines Spatial
    // for that block, consumes the matching ordinal, and keeps the queue alive
    // so the very next audible block can resume normally.
    queue.reset(240u);
    assert(queue.push_reference(reference_frame(240u, 0.0, 0.0, 0.0, 0.0)));
    assert(queue.push_reference(reference_frame(241u, 1.0, -1.0, 2.0, -2.0)));
    assert(!block.consume(queue, 240u, 1u));
    assert(block.last_error() == genesis_selected_source_block_error::no_sources);
    assert(queue.valid());
    assert(queue.size() == 1u);
    assert(block.consume(queue, 241u, 1u));
    assert(block.valid());
    assert(block.source_present(fm1));
    assert(block.source_present(psg0));
    assert(queue.valid());
    assert(queue.size() == 0u);

    // The generated host may carry an explicit provenance bit. Inexact
    // replacement is rejected without mutating or invalidating the reference
    // lane, so that source family can remain on protected quality.
    queue.reset(250u);
    assert(queue.push_reference(reference_frame(250u, 1.0, 1.0, 2.0, 2.0)));
    assert(!queue.replace_source(250u, fm1, 9.0, 8.0, false));
    assert(queue.valid());
    assert(block.consume(queue, 250u, 1u));
    assert(block.sources()[fm1].left[0] == 1.0f);
    assert(block.sources()[fm1].right[0] == 1.0f);

    // A topology change inside one delivered block is not silently represented
    // as a lane appearing/disappearing halfway through the Omniphony block.
    queue.reset(300u);
    auto first = reference_frame(300u, 1.0, 1.0, 2.0, 2.0);
    auto second = reference_frame(301u, 3.0, 3.0, 4.0, 4.0);
    second.source[psg0] = {};
    assert(queue.push_reference(first));
    assert(queue.push_reference(second));
    assert(!block.consume(queue, 300u, 2u));
    assert(block.last_error() == genesis_selected_source_block_error::topology_changed);
    assert(!queue.valid());

    // Exactness and output ordinals are identity constraints, not advisory data.
    queue.reset(400u);
    auto inexact = reference_frame(400u, 1.0, 1.0, 2.0, 2.0);
    inexact.source[fm1].exact = false;
    assert(queue.push_reference(inexact));
    assert(!block.consume(queue, 400u, 1u));
    assert(block.last_error() == genesis_selected_source_block_error::inexact_source);
    assert(!queue.valid());

    queue.reset(501u);
    assert(queue.push_reference(reference_frame(501u, 1.0, 1.0, 2.0, 2.0)));
    assert(!block.consume(queue, 500u, 1u));
    assert(block.last_error() == genesis_selected_source_block_error::ordinal_mismatch);
    assert(!queue.valid());

    // Bad call parameters do not corrupt a coherent queue that has not been
    // consumed yet.
    queue.reset(600u);
    assert(queue.push_reference(reference_frame(600u, 1.0, 1.0, 2.0, 2.0)));
    assert(!block.consume(queue, 600u, 0u));
    assert(block.last_error() == genesis_selected_source_block_error::invalid_frames);
    assert(queue.valid());
    assert(queue.size() == 1u);

    return 0;
}
