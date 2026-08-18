extern "C" {
#include "emu/EmuStructs.h"
#include "emu/cores/nukedopm.h"
}

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {

constexpr std::uint32_t kYm2151Clock = 3579545u;
constexpr std::size_t kChannels = 8u;
constexpr std::size_t kFrames = 8192u;

using stereo_buffer = std::array<std::array<DEV_SMPL, kFrames>, 2>;

template <typename Function>
Function decode_function_pointer(void* raw) {
    static_assert(sizeof(Function) == sizeof(raw),
                  "libvgm function-pointer bridge assumes equal pointer widths");
    Function result{};
    std::memcpy(&result, &raw, sizeof(result));
    return result;
}

DEVFUNC_WRITE_A8D8 find_register_writer() {
    for (const DEVDEF_RWFUNC* rw = devDef_YM2151_Nuked.rwFuncs;
         rw != nullptr && rw->funcPtr != nullptr;
         ++rw) {
        if (rw->funcType == (RWF_REGISTER | RWF_WRITE) && rw->rwType == DEVRW_A8D8)
            return decode_function_pointer<DEVFUNC_WRITE_A8D8>(rw->funcPtr);
    }
    return nullptr;
}

class nuked_opm_instance {
public:
    explicit nuked_opm_instance(std::uint32_t mute_mask)
        : write_(find_register_writer()) {
        if (write_ == nullptr)
            throw std::runtime_error("Nuked-OPM register writer was not exposed");

        DEV_GEN_CFG config{};
        config.emuCore = 0;
        config.srMode = DEVRI_SRMODE_NATIVE;
        config.flags = 0;
        config.clock = kYm2151Clock;
        config.smplRate = 0;

        if (devDef_YM2151_Nuked.Start(&config, &info_) != 0u || info_.dataPtr == nullptr)
            throw std::runtime_error("Nuked-OPM failed to start");
        chip_ = info_.dataPtr->chipInf;
        if (chip_ == nullptr)
            throw std::runtime_error("Nuked-OPM returned no chip state");

        devDef_YM2151_Nuked.Reset(chip_);
        devDef_YM2151_Nuked.SetMuteMask(chip_, mute_mask);
    }

    nuked_opm_instance(const nuked_opm_instance&) = delete;
    nuked_opm_instance& operator=(const nuked_opm_instance&) = delete;

    ~nuked_opm_instance() {
        if (chip_ != nullptr)
            devDef_YM2151_Nuked.Stop(chip_);
    }

    void write(std::uint8_t reg, std::uint8_t value) {
        write_(chip_, reg, value);
    }

    stereo_buffer render() {
        stereo_buffer output{};
        DEV_SMPL* planes[2] = {output[0].data(), output[1].data()};
        devDef_YM2151_Nuked.Update(chip_, static_cast<UINT32>(kFrames), planes);
        return output;
    }

private:
    DEV_INFO info_{};
    void* chip_ = nullptr;
    DEVFUNC_WRITE_A8D8 write_ = nullptr;
};

template <typename Instances>
void broadcast_write(Instances& instances, std::uint8_t reg, std::uint8_t value) {
    for (auto* instance : instances)
        instance->write(reg, value);
}

template <typename Instances>
void program_hot_algorithm7_patch(Instances& instances) {
    // Eight simultaneous channels, four carrier operators each. This deliberately
    // drives the shared mixer/DAC hard enough to expose any non-additive downstream
    // behavior. All nine emulator instances receive the exact same writes.
    for (std::size_t channel = 0; channel < kChannels; ++channel) {
        const auto ch = static_cast<std::uint8_t>(channel);
        broadcast_write(instances, static_cast<std::uint8_t>(0x20u + ch), 0xC7u);
        broadcast_write(instances, static_cast<std::uint8_t>(0x28u + ch), 0x4Au);
        broadcast_write(instances, static_cast<std::uint8_t>(0x30u + ch), 0x00u);
        broadcast_write(instances, static_cast<std::uint8_t>(0x38u + ch), 0x00u);

        for (std::size_t physical_slot = 0; physical_slot < 4u; ++physical_slot) {
            const auto slot = static_cast<std::uint8_t>(channel + physical_slot * 8u);
            broadcast_write(instances, static_cast<std::uint8_t>(0x40u + slot), 0x01u); // DT1=0, MUL=1
            broadcast_write(instances, static_cast<std::uint8_t>(0x60u + slot), 0x00u); // TL=0
            broadcast_write(instances, static_cast<std::uint8_t>(0x80u + slot), 0x1Fu); // AR=max
            broadcast_write(instances, static_cast<std::uint8_t>(0xA0u + slot), 0x00u); // no AM, D1R=0
            broadcast_write(instances, static_cast<std::uint8_t>(0xC0u + slot), 0x00u); // DT2=0, D2R=0
            broadcast_write(instances, static_cast<std::uint8_t>(0xE0u + slot), 0x0Fu); // D1L=0, RR=max
        }
    }

    for (std::size_t channel = 0; channel < kChannels; ++channel) {
        broadcast_write(
            instances,
            0x08u,
            static_cast<std::uint8_t>(0x78u | static_cast<std::uint8_t>(channel)));
    }
}

} // namespace

int main() {
    try {
        nuked_opm_instance full(0x00u);
        std::array<nuked_opm_instance*, kChannels + 1u> all{};
        all[0] = &full;

        std::array<nuked_opm_instance*, kChannels> solo_ptrs{};
        std::array<nuked_opm_instance, kChannels>* impossible_stack_array = nullptr;
        (void)impossible_stack_array;

        // std::array cannot directly construct eight instances with different mute
        // masks, so keep explicit ownership in a small fixed pointer/unique object
        // pattern without introducing heap behavior into the comparison itself.
        nuked_opm_instance solo0(0xFEu);
        nuked_opm_instance solo1(0xFDu);
        nuked_opm_instance solo2(0xFBu);
        nuked_opm_instance solo3(0xF7u);
        nuked_opm_instance solo4(0xEFu);
        nuked_opm_instance solo5(0xDFu);
        nuked_opm_instance solo6(0xBFu);
        nuked_opm_instance solo7(0x7Fu);
        solo_ptrs = {&solo0, &solo1, &solo2, &solo3, &solo4, &solo5, &solo6, &solo7};
        for (std::size_t channel = 0; channel < kChannels; ++channel)
            all[channel + 1u] = solo_ptrs[channel];

        program_hot_algorithm7_patch(all);

        const stereo_buffer full_output = full.render();
        std::array<stereo_buffer, kChannels> solo_output{};
        for (std::size_t channel = 0; channel < kChannels; ++channel)
            solo_output[channel] = solo_ptrs[channel]->render();

        std::size_t audible_full_samples = 0;
        std::array<std::size_t, kChannels> audible_solo_samples{};
        std::size_t mismatch_samples = 0;
        std::int64_t max_abs_delta = 0;
        std::size_t first_mismatch_frame = kFrames;
        std::size_t first_mismatch_plane = 0;

        for (std::size_t plane = 0; plane < 2u; ++plane) {
            for (std::size_t frame = 0; frame < kFrames; ++frame) {
                if (full_output[plane][frame] != 0)
                    ++audible_full_samples;

                std::int64_t summed_solos = 0;
                for (std::size_t channel = 0; channel < kChannels; ++channel) {
                    const DEV_SMPL sample = solo_output[channel][plane][frame];
                    summed_solos += static_cast<std::int64_t>(sample);
                    if (sample != 0)
                        ++audible_solo_samples[channel];
                }

                const std::int64_t full_sample =
                    static_cast<std::int64_t>(full_output[plane][frame]);
                const std::int64_t delta = summed_solos - full_sample;
                const std::int64_t abs_delta = delta < 0 ? -delta : delta;
                if (abs_delta > max_abs_delta)
                    max_abs_delta = abs_delta;
                if (delta != 0) {
                    ++mismatch_samples;
                    if (first_mismatch_frame == kFrames) {
                        first_mismatch_frame = frame;
                        first_mismatch_plane = plane;
                    }
                }
            }
        }

        if (audible_full_samples == 0u) {
            std::cerr << "falsifier invalid: hot full-chip patch produced no audio\n";
            return 2;
        }
        for (std::size_t channel = 0; channel < kChannels; ++channel) {
            if (audible_solo_samples[channel] == 0u) {
                std::cerr << "falsifier invalid: solo channel " << channel
                          << " produced no audio\n";
                return 3;
            }
        }

        if (mismatch_samples == 0u) {
            std::cerr
                << "Nuked-OPM additivity was NOT falsified by the hot patch; "
                   "do not infer separability, strengthen the witness instead\n";
            return 4;
        }

        std::cout << "Nuked-OPM channel additivity falsified: mismatches="
                  << mismatch_samples << ", max_abs_delta=" << max_abs_delta
                  << ", first_frame=" << first_mismatch_frame
                  << ", first_plane=" << first_mismatch_plane << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Nuked-OPM additivity falsifier setup failed: "
                  << error.what() << '\n';
        return 5;
    }
}
