#include "Cheat.h"

#include <windows.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/Hook.h"
#include "core/ModuleManager.h"
#include "core/Offsets.h"
#include "gui/Overlay.h"
#include "modules/movement/Flight.h"
#include "modules/movement/NoClip.h"
#include "modules/movement/NoFall.h"
#include "modules/movement/Speed.h"
#include "modules/movement/Spider.h"
#include "tools/OffsetWizard.h"

namespace cheat {

namespace {

void AttachConsole() {
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    std::printf("cheat.dll attached\n");
}

std::filesystem::path ThisModuleDir() {
    wchar_t modulePath[MAX_PATH]{};
    HMODULE thisModule = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&ThisModuleDir), &thisModule);
    GetModuleFileNameW(thisModule, modulePath, MAX_PATH);
    return std::filesystem::path(modulePath).parent_path();
}

void LoadOffsets(const std::filesystem::path& moduleDir) {
    std::filesystem::path offsetsPath = moduleDir / L"offsets.json";

    if (!core::GetOffsets().LoadFromFile(offsetsPath.wstring())) {
        std::printf(
            "offsets.json not found or invalid next to cheat.dll (%ls) - movement "
            "modules will no-op until it's populated (or found via F5 - Offset Finder).\n",
            offsetsPath.c_str());
    } else if (!core::GetOffsets().IsValid()) {
        std::printf(
            "offsets.json loaded but clientInstanceRva is still zero - fill it in, or "
            "run the F5 Offset Finder in-game.\n");
    }
}

// Reads the marker the injector leaves pointing at the persistent (exe-adjacent)
// offsets.json, so the offset auto-finder has somewhere durable to save its results.
void LoadPersistentConfigPath(const std::filesystem::path& moduleDir) {
    std::ifstream file(moduleDir / L"config_path.txt", std::ios::binary);
    if (!file.is_open()) {
        return;
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    std::string utf8Path = contents.str();
    if (utf8Path.empty()) {
        return;
    }

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) {
        return;
    }
    std::wstring widePath(wideLen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, widePath.data(), wideLen);

    core::GetPersistentConfigPath() = widePath;
}

void RegisterModules() {
    auto& manager = core::ModuleManager::Instance();
    manager.Register<modules::Flight>();
    manager.Register<modules::Speed>();
    manager.Register<modules::NoFall>();
    manager.Register<modules::NoClip>();
    manager.Register<modules::Spider>();
    manager.Register<tools::OffsetWizard>();
}

}  // namespace

void Init() {
    AttachConsole();

    std::filesystem::path moduleDir = ThisModuleDir();
    LoadPersistentConfigPath(moduleDir);
    LoadOffsets(moduleDir);

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
