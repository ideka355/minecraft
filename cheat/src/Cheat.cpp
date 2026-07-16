#include "Cheat.h"

#include <windows.h>

#include <cstdio>
#include <filesystem>

#include "core/Hook.h"
#include "core/ModuleManager.h"
#include "core/Offsets.h"
#include "gui/Overlay.h"
#include "modules/movement/Flight.h"
#include "modules/movement/NoClip.h"
#include "modules/movement/NoFall.h"
#include "modules/movement/Speed.h"
#include "modules/movement/Spider.h"

namespace cheat {

namespace {

void AttachConsole() {
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    std::printf("cheat.dll attached\n");
}

void LoadOffsets() {
    wchar_t modulePath[MAX_PATH]{};
    HMODULE thisModule = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&LoadOffsets), &thisModule);
    GetModuleFileNameW(thisModule, modulePath, MAX_PATH);

    std::filesystem::path offsetsPath =
        std::filesystem::path(modulePath).parent_path() / L"offsets.json";

    if (!core::GetOffsets().LoadFromFile(offsetsPath.wstring())) {
        std::printf(
            "offsets.json not found or invalid next to cheat.dll (%ls) - movement "
            "modules will no-op until it's populated.\n",
            offsetsPath.c_str());
    } else if (!core::GetOffsets().IsValid()) {
        std::printf(
            "offsets.json loaded but required fields are still zero - fill in "
            "clientInstanceRva and positionOffset at minimum.\n");
    }
}

void RegisterModules() {
    auto& manager = core::ModuleManager::Instance();
    manager.Register<modules::Flight>();
    manager.Register<modules::Speed>();
    manager.Register<modules::NoFall>();
    manager.Register<modules::NoClip>();
    manager.Register<modules::Spider>();
}

}  // namespace

void Init() {
    AttachConsole();
    LoadOffsets();

    if (!core::Hook::Init()) {
        std::printf("MinHook init failed\n");
        return;
    }

    RegisterModules();

    if (!gui::Overlay::Install()) {
        std::printf("Failed to install render overlay\n");
    }
}

void Shutdown() {
    gui::Overlay::Uninstall();
    core::Hook::Shutdown();
    FreeConsole();
}

}  // namespace cheat
