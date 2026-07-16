#include <windows.h>

#include "Cheat.h"

namespace {

DWORD WINAPI InitThread(LPVOID module) {
    cheat::Init();
    return 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(module);
            // Do the real init off the loader thread -- DllMain must return quickly and
            // must not call most Win32 APIs (loader lock).
            CreateThread(nullptr, 0, InitThread, module, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            cheat::Shutdown();
            break;
    }
    return TRUE;
}
