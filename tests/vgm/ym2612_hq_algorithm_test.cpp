#include "components/vgm/enhancement/ym2612_hq_algorithm.h"

#include <cassert>
#include <cstddef>

int main() {
    using namespace gameaudio::vgm;

    // The automatic descendant remains exactly the eight source OPN2
    // algorithms. Carrier counts are the familiar 1,1,1,1,2,3,3,4 ladder.
    constexpr std::size_t expected_carriers[8] = {1, 1, 1, 1, 2, 3, 3, 4};
    for (std::size_t algorithm = 0; algorithm < 8; ++algorithm)
        assert(ym2612_hq_carrier_count(algorithm) == expected_carriers[algorithm]);

    // No fifth operator, ninth algorithm or out-of-range route is admitted by
    // the topology contract.
    assert(!ym2612_hq_route_enabled(4, 0, 0));
    assert(!ym2612_hq_route_enabled(0, 6, 0));
    assert(!ym2612_hq_route_enabled(0, 0, 8));
    assert(ym2612_hq_carrier_count(8) == 0);

    // Algorithm 7 is four parallel carriers; algorithm 0 has only OP4 as its
    // carrier. These two endpoints catch accidental table transposition.
    for (std::size_t op = 0; op < 4; ++op)
        assert(ym2612_hq_route_enabled(op, 5, 7));
    assert(!ym2612_hq_route_enabled(0, 5, 0));
    assert(!ym2612_hq_route_enabled(1, 5, 0));
    assert(!ym2612_hq_route_enabled(2, 5, 0));
    assert(ym2612_hq_route_enabled(3, 5, 0));

    return 0;
}
