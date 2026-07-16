#include "core/Hook.h"

#include <MinHook.h>

namespace core {

bool Hook::Init() { return MH_Initialize() == MH_OK; }

void Hook::Shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

bool Hook::Attach(void* target, void* detour, void** original) {
    if (MH_CreateHook(target, detour, original) != MH_OK) {
        return false;
    }
    return MH_EnableHook(target) == MH_OK;
}

bool Hook::Detach(void* target) {
    MH_DisableHook(target);
    return MH_RemoveHook(target) == MH_OK;
}

}  // namespace core
