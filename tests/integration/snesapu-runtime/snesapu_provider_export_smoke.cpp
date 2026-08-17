#include <Windows.h>

#include <cstdint>
#include <iostream>

namespace {

using pre_brr_callback = std::uint32_t (__stdcall *)(
    void*, std::uint32_t, std::uint32_t, std::int16_t*);
using set_pre_brr_provider = void (__stdcall *)(pre_brr_callback, void*);

using studio_begin_callback = std::uint32_t (__stdcall *)(
    void*,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t);
using studio_sample_callback = std::uint32_t (__stdcall *)(
    void*,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    std::uint32_t,
    float*);
using set_studio_source_provider = void (__stdcall *)(
    studio_begin_callback,
    studio_sample_callback,
    void*);

template <typename Function>
Function resolve(HMODULE module, const char* name) noexcept {
    return reinterpret_cast<Function>(GetProcAddress(module, name));
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || argv[1] == nullptr || argv[1][0] == L'\0') {
        std::wcerr << L"usage: snesapu_provider_export_smoke.exe <SNESAPU.dll>\n";
        return 2;
    }

    HMODULE module = LoadLibraryW(argv[1]);
    if (module == nullptr) {
        std::wcerr << L"failed to load SNESAPU.dll, GetLastError="
                   << GetLastError() << L"\n";
        return 3;
    }

    const auto set_pre_brr = resolve<set_pre_brr_provider>(
        module, "SetDSPPreBrrProvider");
    const auto set_studio = resolve<set_studio_source_provider>(
        module, "SetDSPStudioSourceProvider");
    if (set_pre_brr == nullptr || set_studio == nullptr) {
        std::wcerr << L"required SNESAPU provider export is missing\n";
        FreeLibrary(module);
        return 4;
    }

    // Null installation is the provider reset operation used by the real child
    // when no verified source packet is active. Calling it here exercises the
    // exported stdcall entry points without inventing an audio/session state.
    set_pre_brr(nullptr, nullptr);
    set_studio(nullptr, nullptr, nullptr);

    FreeLibrary(module);
    std::wcout << L"SNESAPU provider export ABI smoke passed\n";
    return 0;
}
