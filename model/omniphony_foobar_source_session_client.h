#pragma once

#include "omniphony_dynamic_backend_loader.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace vgmtooling::model {

constexpr std::uint32_t omniphony_foobar_source_session_abi_major_required = 0u;
constexpr std::uint32_t omniphony_foobar_source_session_abi_minor_required = 1u;

// Non-owning client for the process-local source-session exports published by
// foo_out_omniphony.dll. The output component owns final headphone rendering;
// VGM Compiler keeps source truth, causal evidence, and the past-derived
// presentation budget. No chip-specific semantics live at this boundary.
class omniphony_foobar_source_session_client {
public:
    omniphony_foobar_source_session_client() noexcept = default;
    ~omniphony_foobar_source_session_client() noexcept = default;

    omniphony_foobar_source_session_client(
        const omniphony_foobar_source_session_client&) = delete;
    omniphony_foobar_source_session_client& operator=(
        const omniphony_foobar_source_session_client&) = delete;

    bool configure(const omniphony_source_config_transport& config) noexcept {
        if (!omniphony_source_config_valid(config))
            return false;
        if (!configured_ || !same_config(config_, config)) {
            config_ = config;
            configured_ = true;
            advance_epoch();
            reference_stereo_ = nullptr;
            reference_frames_ = 0;
            if (refresh_exports() && reset_ != nullptr)
                (void)reset_(session_epoch_);
        }
        return true;
    }

    bool output_active() noexcept {
#ifdef _WIN32
        return refresh_exports() && output_active_ != nullptr && output_active_() != 0u;
#else
        return false;
#endif
    }

    bool ready() noexcept {
        return configured_ && output_active();
    }

    template <typename Pipeline>
    bool bind(Pipeline& pipeline) noexcept {
        if (!ready())
            return false;
        return pipeline.bind_renderer(
            reinterpret_cast<omniphony_source_processor_handle*>(this),
            &source_abi_major_proxy,
            &source_abi_minor_proxy,
            &reset_proxy,
            &set_mix_budget_proxy,
            &process_events_proxy);
    }

    void set_reference_stereo(const float* stereo, std::size_t frames) noexcept {
        reference_stereo_ = stereo;
        reference_frames_ = stereo == nullptr ? 0u : frames;
    }

    void clear_reference_stereo() noexcept {
        reference_stereo_ = nullptr;
        reference_frames_ = 0;
    }

    void close() noexcept {
#ifdef _WIN32
        module_ = nullptr;
#endif
        abi_major_ = nullptr;
        abi_minor_ = nullptr;
        output_active_ = nullptr;
        reset_ = nullptr;
        publish_ = nullptr;
        clear_reference_stereo();
    }

    std::uint64_t session_epoch() const noexcept { return session_epoch_; }

private:
    using session_abi_version_fn = std::uint32_t (*)();
    using session_output_active_fn = std::uint32_t (*)();
    using session_reset_fn = std::int32_t (*)(std::uint64_t);
    using session_publish_v1_fn = std::int32_t (*)(
        const omniphony_source_config_transport*,
        std::uint64_t,
        const omniphony_source_mix_budget_v1_transport*,
        const float*,
        const omniphony_source_evidence_v1_transport*,
        std::size_t,
        const omniphony_source_evidence_event_v1_transport*,
        std::size_t,
        std::size_t,
        std::uint64_t,
        std::uint32_t,
        const float*);

    static bool same_config(
        const omniphony_source_config_transport& left,
        const omniphony_source_config_transport& right) noexcept
    {
        return left.sample_rate_hz == right.sample_rate_hz
            && left.spatial_mode == right.spatial_mode
            && left.externalization == right.externalization
            && left.hrir_source == right.hrir_source
            && left.unit_scale_m == right.unit_scale_m
            && left.reflection_level == right.reflection_level;
    }

    static bool valid_mix_budget(
        const omniphony_source_mix_budget_v1_transport& budget) noexcept
    {
        const auto bounded = [](float value, float maximum) noexcept {
            return std::isfinite(value) && value >= 0.0f && value <= maximum;
        };
        return bounded(budget.depth_scale, 1.5f)
            && bounded(budget.height_scale, 1.5f)
            && bounded(budget.shared_wet_strength_scale, 1.5f)
            && bounded(budget.shared_wet_extent_scale, 1.5f)
            && bounded(budget.externalization_scale, 1.0f);
    }

    void advance_epoch() noexcept {
        ++session_epoch_;
        if (session_epoch_ == 0u)
            session_epoch_ = 1u;
    }

#ifdef _WIN32
    template <typename Fn>
    Fn resolve(const char* name) const noexcept {
        return reinterpret_cast<Fn>(::GetProcAddress(module_, name));
    }
#endif

    bool refresh_exports() noexcept {
#ifdef _WIN32
        HMODULE loaded = ::GetModuleHandleW(L"foo_out_omniphony.dll");
        if (loaded == nullptr) {
            close();
            return false;
        }
        if (module_ == loaded && abi_major_ != nullptr && abi_minor_ != nullptr &&
            output_active_ != nullptr && reset_ != nullptr && publish_ != nullptr)
            return true;

        module_ = loaded;
        abi_major_ = resolve<session_abi_version_fn>(
            "omniphony_foobar_source_session_abi_major");
        abi_minor_ = resolve<session_abi_version_fn>(
            "omniphony_foobar_source_session_abi_minor");
        output_active_ = resolve<session_output_active_fn>(
            "omniphony_foobar_source_session_output_active");
        reset_ = resolve<session_reset_fn>(
            "omniphony_foobar_source_session_reset");
        publish_ = resolve<session_publish_v1_fn>(
            "omniphony_foobar_source_session_publish_v1");
        if (abi_major_ == nullptr || abi_minor_ == nullptr || output_active_ == nullptr ||
            reset_ == nullptr || publish_ == nullptr ||
            abi_major_() != omniphony_foobar_source_session_abi_major_required ||
            abi_minor_() < omniphony_foobar_source_session_abi_minor_required) {
            close();
            return false;
        }
        return true;
#else
        return false;
#endif
    }

    static omniphony_foobar_source_session_client* self(
        omniphony_source_processor_handle* processor) noexcept
    {
        return reinterpret_cast<omniphony_foobar_source_session_client*>(processor);
    }

    static std::uint32_t source_abi_major_proxy() noexcept {
        return omniphony_source_abi_major_required;
    }

    static std::uint32_t source_abi_minor_proxy() noexcept {
        return omniphony_source_abi_minor_required;
    }

    static std::int32_t reset_proxy(
        omniphony_source_processor_handle* processor) noexcept
    {
        auto* client = self(processor);
        if (client == nullptr)
            return -30;
        client->advance_epoch();
        client->clear_reference_stereo();
        if (!client->refresh_exports() || client->reset_ == nullptr)
            return -31;
        return client->reset_(client->session_epoch_);
    }

    static std::int32_t set_mix_budget_proxy(
        omniphony_source_processor_handle* processor,
        const omniphony_source_mix_budget_v1_transport* budget) noexcept
    {
        auto* client = self(processor);
        if (client == nullptr || budget == nullptr || !valid_mix_budget(*budget))
            return -32;
        client->mix_budget_ = *budget;
        return 0;
    }

    static std::int32_t process_events_proxy(
        omniphony_source_processor_handle* processor,
        const float* input,
        const omniphony_source_evidence_v1_transport* sources,
        std::size_t source_count,
        const omniphony_source_evidence_event_v1_transport* events,
        std::size_t event_count,
        std::size_t frames,
        std::uint64_t sample_pos,
        std::uint32_t ramp_frames,
        float* output) noexcept
    {
        auto* client = self(processor);
        if (client == nullptr || !client->configured_ || input == nullptr || sources == nullptr ||
            source_count == 0u || output == nullptr || client->reference_stereo_ == nullptr ||
            client->reference_frames_ != frames ||
            (event_count != 0u && events == nullptr)) {
            if (client != nullptr)
                client->clear_reference_stereo();
            return -33;
        }
        if (!client->refresh_exports() || client->output_active_ == nullptr ||
            client->output_active_() == 0u || client->publish_ == nullptr) {
            client->clear_reference_stereo();
            return -34;
        }

        const float* reference = client->reference_stereo_;
        const std::int32_t status = client->publish_(
            &client->config_,
            client->session_epoch_,
            &client->mix_budget_,
            input,
            sources,
            source_count,
            events,
            event_count,
            frames,
            sample_pos,
            ramp_frames,
            reference);
        if (status == 0) {
            for (std::size_t sample = 0; sample < frames * 2u; ++sample)
                output[sample] = reference[sample];
        }
        client->clear_reference_stereo();
        return status;
    }

    omniphony_source_config_transport config_{};
    omniphony_source_mix_budget_v1_transport mix_budget_{};
    const float* reference_stereo_ = nullptr;
    std::size_t reference_frames_ = 0;
    std::uint64_t session_epoch_ = 1u;
    bool configured_ = false;
#ifdef _WIN32
    HMODULE module_ = nullptr;
#endif
    session_abi_version_fn abi_major_ = nullptr;
    session_abi_version_fn abi_minor_ = nullptr;
    session_output_active_fn output_active_ = nullptr;
    session_reset_fn reset_ = nullptr;
    session_publish_v1_fn publish_ = nullptr;
};

} // namespace vgmtooling::model
