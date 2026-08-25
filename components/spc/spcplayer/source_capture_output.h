#pragma once

#include "spcplayer.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

class snesapu_source_capture_api
{
public:
    using set_capture_fn = void (__stdcall *)(std::uint32_t enable);
    using get_source_fn = float* (__stdcall *)(std::uint32_t* frames);

    bool resolve() noexcept
    {
        HMODULE module = GetModuleHandleW(L"SNESAPU.dll");
        if (!module)
            module = GetModuleHandleW(L"snesapu.dll");
        if (!module)
            return false;

        m_set_capture = reinterpret_cast<set_capture_fn>(
            GetProcAddress(module, "SetDSPSourceCapture"));
        m_get_source = reinterpret_cast<get_source_fn>(
            GetProcAddress(module, "GetDSPSourceData"));
        return m_set_capture && m_get_source;
    }

    bool available() const noexcept
    {
        return m_set_capture && m_get_source;
    }

    void set_enabled(bool enabled) const
    {
        if (!available())
            throw std::runtime_error("SNESAPU causal source exports are unavailable");
        m_set_capture(enabled ? 1u : 0u);
    }

    const float* completed_block(std::uint32_t expected_frames) const
    {
        if (!available())
            throw std::runtime_error("SNESAPU causal source exports are unavailable");
        std::uint32_t frames = 0;
        float* ptr = m_get_source(&frames);
        if (!ptr || frames != expected_frames)
            throw std::runtime_error("SNESAPU source block does not match reference PCM frame count");
        return ptr;
    }

private:
    set_capture_fn m_set_capture = nullptr;
    get_source_fn m_get_source = nullptr;
};

// SNESAPU writes one sample-major scratch frame with the SRCE v2 planes. The
// process wire is planar because the x64 parent and Omniphony consume
// source-major vectors. Audio planes are normalized from the DSP's approximately
// 16-bit internal scale; effective route-coefficient planes remain native floats.
inline void write_source_block(
    const float* sample_major,
    std::uint32_t frames,
    std::vector<float>& planar_scratch)
{
    using wire = gameaudio::spc::snesapu_source_wire_v2;

    if (frames > wire::max_frames)
        throw std::runtime_error("source block exceeds SNESAPU MIX_SIZE");

    planar_scratch.assign(wire::plane_count * static_cast<std::size_t>(frames), 0.0f);

    if (sample_major)
    {
        constexpr float k_audio_scale = 1.0f / 32768.0f;
        for (std::size_t plane = 0; plane < wire::plane_count; ++plane)
        {
            const float scale = wire::is_audio_plane(plane) ? k_audio_scale : 1.0f;
            float* dst = planar_scratch.data() + plane * static_cast<std::size_t>(frames);
            for (std::uint32_t frame = 0; frame < frames; ++frame)
            {
                dst[frame] = sample_major[
                    static_cast<std::size_t>(frame) * wire::plane_count + plane] * scale;
            }
        }
    }

    wire::header header{};
    header.magic = wire::magic;
    header.version = wire::version;
    header.header_size = static_cast<std::uint16_t>(sizeof(header));
    header.block_samples = frames;
    header.plane_count = static_cast<std::uint16_t>(wire::plane_count);
    header.sample_format = wire::format_float32;
    header.audio_lanes = static_cast<std::uint16_t>(wire::audio_lane_count);

    if (!wire::header_valid(header))
        throw std::runtime_error("constructed invalid SNESAPU SRCE v2 header");

    std::cout.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!planar_scratch.empty())
    {
        std::cout.write(
            reinterpret_cast<const char*>(planar_scratch.data()),
            static_cast<std::streamsize>(planar_scratch.size() * sizeof(float)));
    }
    if (!std::cout.good())
        throw std::runtime_error("failed to write SRCE source block");
}
