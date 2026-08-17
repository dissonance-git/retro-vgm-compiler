#include "model/spatial_source_host_session.h"

#include <cstdint>
#include <limits>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static spatial_source_evidence make_evidence(std::uint64_t id, std::uint64_t generation) {
    spatial_source_evidence value;
    value.family = spatial_source_family::vgm;
    value.source_id = id;
    value.generation = generation;
    return value;
}

int main() {
    {
        spatial_source_host_session<1, 16, 16> session;
        float first_pcm[4] = {1, 2, 3, 4};
        spatial_audio_lane_view first_lane{
            spatial_audio_lane_kind::dry_source,
            first_pcm,
            make_evidence(1, 1),
        };

        CHECK(!session.push_at(1000, spatial_source_block_view{&first_lane, 1, 4}));
        CHECK(session.last_error() == spatial_source_host_session_error::inactive);

        session.reset(spatial_source_host_discontinuity::initialize, 1000);
        CHECK(session.active());
        CHECK(session.session_epoch() == 1);
        CHECK(session.push_at(1000, spatial_source_block_view{&first_lane, 1, 4}));
        CHECK(session.expected_push_frame() == 1004);

        auto a = session.pull(2);
        CHECK(a.sources.frame_count == 2);
        CHECK(a.reference_frame_start == 1000);
        CHECK(a.session_epoch == 1);
        CHECK(session.next_pull_frame() == 1002);

        auto b = session.pull(2);
        CHECK(b.sources.frame_count == 2);
        CHECK(b.reference_frame_start == 1002);
        CHECK(b.session_epoch == 1);
        CHECK(session.buffered_frames() == 0);
    }

    {
        spatial_source_host_session<1, 16, 16> session;
        session.reset(spatial_source_host_discontinuity::initialize, 0);
        float a_pcm[4] = {1, 2, 3, 4};
        spatial_audio_lane_view a_lane{
            spatial_audio_lane_kind::dry_source,
            a_pcm,
            make_evidence(2, 1),
        };
        CHECK(session.push_at(0, spatial_source_block_view{&a_lane, 1, 4}));
        auto prefix = session.pull(1);
        CHECK(prefix.sources.frame_count == 1);
        CHECK(session.buffered_frames() == 3);

        session.reset(spatial_source_host_discontinuity::seek, 9000);
        CHECK(session.session_epoch() == 2);
        CHECK(session.last_discontinuity() == spatial_source_host_discontinuity::seek);
        CHECK(session.discarded_frames_total() == 3);
        CHECK(session.buffered_frames() == 0);
        CHECK(session.next_pull_frame() == 9000);

        float b_pcm[2] = {9, 10};
        spatial_audio_lane_view b_lane{
            spatial_audio_lane_kind::dry_source,
            b_pcm,
            make_evidence(3, 1),
        };
        CHECK(session.push_at(9000, spatial_source_block_view{&b_lane, 1, 2}));
        auto after_seek = session.pull(2);
        CHECK(after_seek.reference_frame_start == 9000);
        CHECK(after_seek.session_epoch == 2);
        CHECK(after_seek.sources.lanes[0].evidence.source_id == 3);
    }

    {
        // A gap or overlap is evidence of a host/decoder discontinuity. Reject it
        // without advancing the accepted timeline; the caller must reset first.
        spatial_source_host_session<1, 8, 8> session;
        session.reset(spatial_source_host_discontinuity::initialize, 50);
        float pcm[2] = {1, 2};
        spatial_audio_lane_view lane{
            spatial_audio_lane_kind::dry_source,
            pcm,
            make_evidence(4, 1),
        };
        CHECK(session.push_at(50, spatial_source_block_view{&lane, 1, 2}));
        CHECK(!session.push_at(53, spatial_source_block_view{&lane, 1, 2}));
        CHECK(session.last_error() == spatial_source_host_session_error::noncontiguous_input);
        CHECK(session.expected_push_frame() == 52);
        CHECK(session.buffered_frames() == 2);
    }

    {
        // The reference timeline is checked before the assembler mutates state.
        spatial_source_host_session<1, 8, 8> session;
        const auto max = std::numeric_limits<std::uint64_t>::max();
        session.reset(spatial_source_host_discontinuity::initialize, max - 1);
        float pcm[2] = {1, 2};
        spatial_audio_lane_view lane{
            spatial_audio_lane_kind::dry_source,
            pcm,
            make_evidence(5, 1),
        };
        CHECK(!session.push_at(max - 1, spatial_source_block_view{&lane, 1, 2}));
        CHECK(session.last_error() == spatial_source_host_session_error::reference_timeline_overflow);
        CHECK(session.buffered_frames() == 0);
    }

    {
        // Identity boundaries produced by the assembler keep correct reference
        // time on both sides of the split.
        spatial_source_host_session<1, 16, 16> session;
        session.reset(spatial_source_host_discontinuity::initialize, 200);
        float a_pcm[3] = {1, 2, 3};
        float b_pcm[3] = {4, 5, 6};
        spatial_audio_lane_view a_lane{
            spatial_audio_lane_kind::dry_source,
            a_pcm,
            make_evidence(6, 1),
        };
        spatial_audio_lane_view b_lane{
            spatial_audio_lane_kind::dry_source,
            b_pcm,
            make_evidence(7, 1),
        };
        CHECK(session.push_at(200, spatial_source_block_view{&a_lane, 1, 3}));
        CHECK(session.push_at(203, spatial_source_block_view{&b_lane, 1, 3}));
        auto first = session.pull(6);
        CHECK(first.sources.frame_count == 3);
        CHECK(first.reference_frame_start == 200);
        CHECK(session.last_pull_identity_limited());
        auto second = session.pull(6);
        CHECK(second.sources.frame_count == 3);
        CHECK(second.reference_frame_start == 203);
        CHECK(second.sources.lanes[0].evidence.source_id == 7);
    }

    return 0;
}
