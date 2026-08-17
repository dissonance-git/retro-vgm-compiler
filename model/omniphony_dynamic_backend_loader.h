#pragma once

#include "realtime_musical_omniphony_pipeline.h"

#include <cmath>
#include <cstdint>
#include <type_traits>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace vgmtooling::model {

constexpr std::uint32_t omniphony_source_spatial_native_routing = 0u;
constexpr std::uint32_t omniphony_source_spatial_full_sphere = 1u;
constexpr std::uint32_t omniphony_source_hrir_saf_kemar = 0u;
constexpr std::uint32_t omniphony_source_hrir_synthetic = 1u;

// C-layout mirror of Omniphony source_ffi's OmniphonySourceConfig. Keep this
// small lifecycle/config surface beside the loader while the portable source
// transport remains independent of platform loading.
struct omniphony_source_config_transport {
    std::uint32_t sample_rate_hz = 0;
    std::uint32_t spatial_mode = omniphony_source_spatial_native_routing;
    std::uint32_t externalization = 0;
    std::uint32_t hrir_source = omniphony_source_hrir_saf_kemar;
    float unit_scale_m = 0.0f;
    float reflection_level = 0.0f;
};

static_assert(std::is_standard_layout_v<omniphony_source_config_transport>);
static_assert(sizeof(omniphony_source_config_transport) == 24u);

using omniphony_source_create_fn = omniphony_source_processor_handle* (*)(
    const omniphony_source_config_transport*);
using omniphony_source_destroy_fn = void (*)(omniphony_source_processor_handle*);

enum class omniphony_dynamic_backend_error : std::uint8_t {
    none = 0,
    unsupported_platform,
    invalid_config,
    library_load_failed,
    missing_symbol,
    abi_mismatch,
    processor_create_failed,
};

inline bool omniphony_source_config_valid(
    const omniphony_source_config_transport& config) noexcept
{
    return config.sample_rate_hz != 0u
        && config.spatial_mode <= omniphony_source_spatial_full_sphere
        && config.externalization <= 1u
        && config.hrir_source <= omniphony_source_hrir_synthetic
        && std::isfinite(config.unit_scale_m)
        && config.unit_scale_m > 0.0f
        && std::isfinite(config.reflection_level)
        && config.reflection_level >= 0.0f;
}

// Owns only the platform/DLL and Omniphony processor lifetime. It has no source
// quality policy and no musical state. Callers resolve their reference versus
// higher-quality source lanes before they ever reach this object.
class omniphony_dynamic_backend_loader {
public:
    omniphony_dynamic_backend_loader() noexcept = default;
    ~omniphony_dynamic_backend_loader() noexcept { close(); }

    omniphony_dynamic_backend_loader(const omniphony_dynamic_backend_loader&) = delete;
    omniphony_dynamic_backend_loader& operator=(const omniphony_dynamic_backend_loader&) = delete;

    bool open_default(const omniphony_source_config_transport& config) noexcept {
        return open(L"omniphony_source.dll", config);
    }

    bool open(
        const wchar_t* library_path,
        const omniphony_source_config_transport& config) noexcept
    {
        close();
        if (!omniphony_source_config_valid(config))
            return fail(omniphony_dynamic_backend_error::invalid_config);

#ifdef _WIN32
        if (library_path == nullptr || library_path[0] == L'\0')
            return fail(omniphony_dynamic_backend_error::library_load_failed);

        module_ = ::LoadLibraryW(library_path);
        if (module_ == nullptr)
            return fail(omniphony_dynamic_backend_error::library_load_failed);

        abi_major_ = resolve<omniphony_source_abi_version_fn>("omniphony_source_abi_major");
        abi_minor_ = resolve<omniphony_source_abi_version_fn>("omniphony_source_abi_minor");
        create_ = resolve<omniphony_source_create_fn>("omniphony_source_create");
        destroy_ = resolve<omniphony_source_destroy_fn>("omniphony_source_destroy");
        reset_ = resolve<omniphony_source_reset_fn>("omniphony_source_reset");
        process_events_ = resolve<omniphony_source_process_events_f32_fn>(
            "omniphony_source_process_events_f32");

        if (abi_major_ == nullptr || abi_minor_ == nullptr || create_ == nullptr ||
            destroy_ == nullptr || reset_ == nullptr || process_events_ == nullptr) {
            close();
            return fail(omniphony_dynamic_backend_error::missing_symbol);
        }

        if (abi_major_() != omniphony_source_abi_major_required ||
            abi_minor_() < omniphony_source_abi_minor_required) {
            close();
            return fail(omniphony_dynamic_backend_error::abi_mismatch);
        }

        processor_ = create_(&config);
        if (processor_ == nullptr) {
            close();
            return fail(omniphony_dynamic_backend_error::processor_create_failed);
        }

        last_error_ = omniphony_dynamic_backend_error::none;
        return true;
#else
        (void)library_path;
        return fail(omniphony_dynamic_backend_error::unsupported_platform);
#endif
    }

    void close() noexcept {
#ifdef _WIN32
        if (processor_ != nullptr && destroy_ != nullptr)
            destroy_(processor_);
        processor_ = nullptr;
        abi_major_ = nullptr;
        abi_minor_ = nullptr;
        create_ = nullptr;
        destroy_ = nullptr;
        reset_ = nullptr;
        process_events_ = nullptr;
        if (module_ != nullptr)
            ::FreeLibrary(module_);
        module_ = nullptr;
#else
        processor_ = nullptr;
        abi_major_ = nullptr;
        abi_minor_ = nullptr;
        create_ = nullptr;
        destroy_ = nullptr;
        reset_ = nullptr;
        process_events_ = nullptr;
#endif
        last_error_ = omniphony_dynamic_backend_error::none;
    }

    bool open() const noexcept {
        return processor_ != nullptr && abi_major_ != nullptr && abi_minor_ != nullptr &&
            reset_ != nullptr && process_events_ != nullptr;
    }

    omniphony_dynamic_backend_error last_error() const noexcept { return last_error_; }
    omniphony_source_processor_handle* processor() const noexcept { return processor_; }

    template <typename Pipeline>
    bool bind(Pipeline& pipeline) const noexcept {
        if (!open())
            return false;
        return pipeline.bind_renderer(
            processor_,
            abi_major_,
            abi_minor_,
            reset_,
            process_events_);
    }

private:
#ifdef _WIN32
    template <typename Fn>
    Fn resolve(const char* name) noexcept {
        return reinterpret_cast<Fn>(::GetProcAddress(module_, name));
    }
#endif

    bool fail(omniphony_dynamic_backend_error error) noexcept {
        last_error_ = error;
        return false;
    }

#ifdef _WIN32
    HMODULE module_ = nullptr;
#endif
    omniphony_source_processor_handle* processor_ = nullptr;
    omniphony_source_abi_version_fn abi_major_ = nullptr;
    omniphony_source_abi_version_fn abi_minor_ = nullptr;
    omniphony_source_create_fn create_ = nullptr;
    omniphony_source_destroy_fn destroy_ = nullptr;
    omniphony_source_reset_fn reset_ = nullptr;
    omniphony_source_process_events_f32_fn process_events_ = nullptr;
    omniphony_dynamic_backend_error last_error_ = omniphony_dynamic_backend_error::none;
};

} // namespace vgmtooling::model
