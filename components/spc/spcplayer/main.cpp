/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <vector>
#include <optional>
#include <limits>
#include <TCHAR.h>
#include <io.h>
#include <fcntl.h>
#include <snesapu.h>
#include "spcplayer.h"
#include "source_capture_output.h"

#ifdef _UNICODE
#define tcerr wcerr
#else
#define tcerr cerr
#endif

static const TCHAR *my_name = _T("spcplayer");
static const TCHAR *my_version = _T("1.1");

static uint32_t get_le32(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] | (((uint32_t)p_data[1]) << 8) |
        (((uint32_t)p_data[2]) << 16) | (((uint32_t)p_data[3]) << 24);
}

static inline uint_fast64_t spctime_to_samples(uint64_t spctime, uint32_t samplerate)
{
    const uint_fast64_t temp = spctime * static_cast<uint_fast64_t>(samplerate);
    return temp >= 64000 ? temp / 64000 : 0;
}

static bool _tcstoul_optional(std::optional<uint32_t>& out, const TCHAR* string, TCHAR** endptr, int radix)
{
    unsigned long ret = _tcstoul(string, endptr, radix);
    if (string == *endptr)
        return false;
    out = ret;
    return true;
}

static void write_telemetry(const Voice* p_voice, const DSPReg* p_dsp, uint32_t samples)
{
    SpcBlockTelem telem{};
    telem.magic = SPCP_TELEM_MAGIC;
    telem.block_samples = samples;

    telem.echo.evolL = p_dsp->evolL;
    telem.echo.evolR = p_dsp->evolR;
    telem.echo.efb = p_dsp->efb;
    telem.echo.edl = p_dsp->edl;
    telem.echo.eon_mask = p_dsp->eon;
    telem.echo.flg = p_dsp->flg;

    for (int i = 0; i < SPCP_TELEM_VOICES; i++)
    {
        SpcVoiceTelem& vt = telem.voice[i];
        vt.mFlg = p_voice[i].mFlg;
        vt.eMode = p_voice[i].eMode;
        vt.is_echo = (p_dsp->eon >> i) & 1;
        vt.is_noise = (p_dsp->non >> i) & 1;
        vt.volL = p_dsp->voice[i].volL;
        vt.volR = p_dsp->voice[i].volR;
        vt.envx = p_dsp->voice[i].envx;
        vt.outx = p_dsp->voice[i].outx;
        vt.pitch = p_dsp->voice[i].pitch;
        vt.srcn = p_dsp->voice[i].srcn;
        vt.eVal = p_voice[i].eVal;
    }

    std::cout.write(reinterpret_cast<const char*>(&telem), sizeof(telem));
    if (!std::cout.good())
        throw std::runtime_error("failed to write SPC telemetry block");
}

int _tmain(int argc, const TCHAR* argv[])
{
    try
    {
        if (argc < 2)
        {
            std::tcerr << my_name << _T(" version ") << my_version << std::endl;
            std::tcerr << _T("Plays SPC files using the project SNESAPU runtime") << std::endl;
            std::tcerr << _T("Reads input data from stdin and writes framed output data to stdout") << std::endl;
            std::tcerr << std::endl;
            std::tcerr << _T("Usage: ") << my_name << _T(".exe [options]") << std::endl;
            std::tcerr << std::endl;
            std::tcerr << _T("Options:") << std::endl;
            std::tcerr << _T("       --time       Song length [1/64000 sec]") << std::endl;
            std::tcerr << _T("       --fade       Fade length [1/64000 sec]") << std::endl;
            std::tcerr << _T("       --rate       Sample rate") << std::endl;
            std::tcerr << _T("       --bits       Bits per sample") << std::endl;
            std::tcerr << _T("       --numchn     Channels") << std::endl;
            std::tcerr << _T("       --dspopts    DSP options") << std::endl;
            std::tcerr << _T("       --inter      Interpolation (0 none, 1 linear, 2 cubic, 3 Gaussian, 4 sinc, 7 Gaussian4)") << std::endl;
            std::tcerr << _T("       --amp        Volume") << std::endl;
            std::tcerr << _T("       --mute       Voice mute mask") << std::endl;
            std::tcerr << _T("       --silence    Continue emitting silence after time+fade") << std::endl;
            std::tcerr << _T("       --telemetry  Emit per-voice telemetry after each PCM block") << std::endl;
            std::tcerr << _T("       --sources    Emit SRCE v2 causal source/control planes; implies telemetry") << std::endl;
            return EXIT_FAILURE;
        }

        std::optional<uint32_t> time;
        std::optional<uint32_t> fade;
        std::optional<uint32_t> rate;
        std::optional<uint32_t> bits;
        std::optional<uint32_t> numchn;
        std::optional<uint32_t> dspopts;
        std::optional<uint32_t> inter;
        std::optional<uint32_t> amp;
        std::optional<uint32_t> mute;
        bool silence = false;
        bool telemetry = false;
        bool sources = false;

        for (int i = 1; i < argc; i++)
        {
            TCHAR* end = nullptr;
            if (_T('-') != argv[i][0] || _T('-') != argv[i][1])
                continue;

            const TCHAR* option = &argv[i][2];
            auto consume_value = [&](std::optional<uint32_t>& out)
            {
                if (i + 1 < argc)
                {
                    ++i;
                    _tcstoul_optional(out, argv[i], &end, 0);
                }
            };

            if (!_tcscmp(option, _T("rate"))) consume_value(rate);
            else if (!_tcscmp(option, _T("bits"))) consume_value(bits);
            else if (!_tcscmp(option, _T("numchn"))) consume_value(numchn);
            else if (!_tcscmp(option, _T("dspopts"))) consume_value(dspopts);
            else if (!_tcscmp(option, _T("time"))) consume_value(time);
            else if (!_tcscmp(option, _T("fade"))) consume_value(fade);
            else if (!_tcscmp(option, _T("inter"))) consume_value(inter);
            else if (!_tcscmp(option, _T("amp"))) consume_value(amp);
            else if (!_tcscmp(option, _T("mute"))) consume_value(mute);
            else if (!_tcscmp(option, _T("silence"))) silence = true;
            else if (!_tcscmp(option, _T("telemetry"))) telemetry = true;
            else if (!_tcscmp(option, _T("sources")))
            {
                sources = true;
                telemetry = true;
            }
        }

        if (!rate || rate < 8000 || rate > 192000) rate = 32000;
        if (!bits || bits < 8 || bits > 32) bits = 16;
        if (!numchn || numchn < 1 || numchn > 2) numchn = 2;
        if (!dspopts) dspopts = 0;
        if (!inter || inter > 7) inter = 3;
        if (!amp) amp = 65536;
        if (!mute) mute = 0;

        bool infinite = false;
        if (time)
        {
            if (!fade) fade = 5 * 64000;
        }
        else
        {
            infinite = true;
        }

        _setmode(_fileno(stdin), _O_BINARY);

        std::vector<uint8_t> spcp_data;
        uint8_t buffer[1024];
        while (std::cin.good())
        {
            std::cin.read(reinterpret_cast<char*>(buffer), std::size(buffer));
            spcp_data.insert(spcp_data.end(), buffer, buffer + std::cin.gcount());
        }

        if (spcp_data.size() < SPCP_HEADER_SIZE)
        {
            std::tcerr << _T("SPCP data size is too small.") << std::endl;
            return EXIT_FAILURE;
        }
        if (strncmp(reinterpret_cast<const char*>(spcp_data.data()), SPCP_HEADER_SIGNATURE, strlen(SPCP_HEADER_SIGNATURE)))
        {
            std::tcerr << _T("SPCP signature does not exist.") << std::endl;
            return EXIT_FAILURE;
        }

        const uint32_t version = get_le32(spcp_data.data() + 4);
        uint32_t header_size = get_le32(spcp_data.data() + 4 * 2);
        uint32_t spc_size = get_le32(spcp_data.data() + 4 * 3);
        uint32_t script700_size = get_le32(spcp_data.data() + 4 * 4);
        (void)version;

        std::vector<uint8_t> spc_data;
        std::vector<uint8_t> script700_data;

        if (header_size + spc_size > spcp_data.size())
            spc_size = static_cast<uint32_t>(spcp_data.size() - header_size);
        spc_data.resize(spc_size);
        memcpy(spc_data.data(), spcp_data.data() + header_size, spc_size);

        if (script700_size)
        {
            if (header_size + spc_size + script700_size > spcp_data.size())
                script700_size = static_cast<uint32_t>(spcp_data.size() - spc_size - header_size);
            script700_data.resize(script700_size + 1);
            memcpy(script700_data.data(), spcp_data.data() + header_size + spc_size, script700_size);
            script700_data[script700_size] = '\0';
        }

        const char* spc_signature = "SNES-SPC700 Sound File Data ";
        const size_t spc_signature_size = strlen(spc_signature);
        if (spc_data.size() < spc_signature_size ||
            strncmp(reinterpret_cast<const char*>(spc_data.data()), spc_signature, spc_signature_size))
        {
            std::tcerr << _T("SPC data is invalid.") << std::endl;
            return EXIT_FAILURE;
        }

        Voice* p_voice = nullptr;
        DSPReg* p_dsp = nullptr;
        u32* p_vMMaxL = nullptr;
        u32* p_vMMaxR = nullptr;
        GetAPUData(nullptr, nullptr, nullptr, nullptr, &p_dsp, &p_voice, &p_vMMaxL, &p_vMMaxR);

        if (script700_size) SetScript700(script700_data.data());
        else SetScript700(nullptr);

        LoadSPCFile(spc_data.data());
        SetAPUOpt(3, numchn.value(), bits.value(), rate.value(), inter.value(), dspopts.value());
        SetDSPAmp(amp.value());

        for (size_t i = 0; i < 8; i++)
        {
            p_voice[i].mFlg = ((mute.value() >> i) & 0x01) ? 1 : 0;
            p_voice[i].vMaxL = p_voice[i].vMaxR = 0;
        }
        *p_vMMaxL = *p_vMMaxR = 0;

        if (!infinite)
            SetAPULength(time.value(), fade.value());

        snesapu_source_capture_api source_api;
        if (sources)
        {
            if (!source_api.resolve())
            {
                std::tcerr << _T("--sources requires patched SNESAPU exports SetDSPSourceCapture/GetDSPSourceData") << std::endl;
                return EXIT_FAILURE;
            }
            source_api.set_enabled(true);
        }

        _setmode(_fileno(stdout), _O_BINARY);

        const uint32_t stream_block_samples = spcp_stream_block_samples(rate.value(), sources);
        const uint32_t bytes_per_frame = numchn.value() * (bits.value() / 8);
        std::vector<uint8_t> decode_buf(static_cast<size_t>(stream_block_samples) * bytes_per_frame);
        std::vector<float> source_planar;

        uint64_t played_samples = 0;
        const uint64_t total_samples = infinite
            ? std::numeric_limits<uint64_t>::max()
            : spctime_to_samples(static_cast<uint64_t>(time.value()) + fade.value(), rate.value());

        auto emit_block = [&](uint32_t frames, bool silent_source_block)
        {
            if (frames == 0) return;
            EmuAPU(decode_buf.data(), frames, 1);
            std::cout.write(
                reinterpret_cast<char*>(decode_buf.data()),
                static_cast<std::streamsize>(static_cast<size_t>(frames) * bytes_per_frame));
            if (!std::cout.good())
                throw std::runtime_error("failed to write SPC reference PCM block");

            if (telemetry && p_voice && p_dsp)
                write_telemetry(p_voice, p_dsp, frames);

            if (sources)
            {
                const float* raw = silent_source_block ? nullptr : source_api.completed_block(frames);
                write_source_block(raw, frames, source_planar);
            }
        };

        while (infinite || played_samples < total_samples)
        {
            uint32_t frames = stream_block_samples;
            if (!infinite)
            {
                const uint64_t remaining = total_samples - played_samples;
                if (remaining < frames)
                    frames = static_cast<uint32_t>(remaining);
            }
            emit_block(frames, false);
            if (!infinite) played_samples += frames;
        }

        if (silence)
        {
            SetDSPAmp(0);
            if (sources)
                source_api.set_enabled(false);
            while (true)
                emit_block(stream_block_samples, true);
        }

        if (sources)
            source_api.set_enabled(false);
    }
    catch (const std::exception& e)
    {
        std::cerr << "spcplayer: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::tcerr << _T("An unexpected error has occurred!") << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
