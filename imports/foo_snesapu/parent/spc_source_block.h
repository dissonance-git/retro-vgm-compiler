#pragma once

#include "../../spcplayer/spcplayer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

struct spc_source_block
{
    uint32_t frames = 0;
    std::vector<float> planar;

    void clear()
    {
        frames = 0;
        planar.clear();
    }

    void resize(uint32_t new_frames)
    {
        frames = new_frames;
        planar.assign(
            static_cast<size_t>(SPCP_SOURCE_PLANES) * new_frames,
            0.0f);
    }

    float* plane(uint32_t index)
    {
        return planar.data() + static_cast<size_t>(index) * frames;
    }

    const float* plane(uint32_t index) const
    {
        return planar.data() + static_cast<size_t>(index) * frames;
    }

    bool valid() const noexcept
    {
        return frames > 0
            && planar.size() == static_cast<size_t>(SPCP_SOURCE_PLANES) * frames;
    }

    void copy_slice_from(
        const spc_source_block& src,
        uint32_t src_offset,
        uint32_t dst_offset,
        uint32_t count)
    {
        if (src_offset + count > src.frames || dst_offset + count > frames)
            return;
        for (uint32_t p = 0; p < SPCP_SOURCE_PLANES; ++p)
        {
            std::copy_n(
                src.plane(p) + src_offset,
                count,
                plane(p) + dst_offset);
        }
    }
};
