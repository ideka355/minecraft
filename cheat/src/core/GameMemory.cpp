#include "core/GameMemory.h"

#include <windows.h>

#include "core/Offsets.h"

namespace core {

namespace {

// Reads a pointer at `address`, guarding against unmapped memory instead of crashing the
// host process on a bad offset.
bool SafeReadPointer(uintptr_t address, uintptr_t* out) {
    if (address == 0) {
        return false;
    }
    __try {
        *out = *reinterpret_cast<uintptr_t*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

uintptr_t GetLocalPlayer() {
    const Offsets& offsets = GetOffsets();
    if (!offsets.IsValid()) {
        return 0;
    }

    uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    uintptr_t address = moduleBase + offsets.clientInstanceRva;

    // First hop: dereference the static pointer slot itself to get the ClientInstance*.
    if (!SafeReadPointer(address, &address)) {
        return 0;
    }

    // Remaining hops: add a byte offset, dereference, repeat -- e.g.
    // ClientInstance* -> (+ offset) -> LocalPlayer*.
    for (uintptr_t hop : offsets.localPlayerChain) {
        if (hop == 0) {
            break;
        }
        if (!SafeReadPointer(address + hop, &address)) {
            return 0;
        }
    }

    return address;
}

bool ReadFloat(uintptr_t address, float* out) {
    if (address == 0) return false;
    __try {
        *out = *reinterpret_cast<float*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool WriteFloat(uintptr_t address, float value) {
    if (address == 0) return false;
    __try {
        *reinterpret_cast<float*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadBool(uintptr_t address, bool* out) {
    if (address == 0) return false;
    __try {
        *out = *reinterpret_cast<bool*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool WriteBool(uintptr_t address, bool value) {
    if (address == 0) return false;
    __try {
        *reinterpret_cast<bool*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadVec3(uintptr_t address, Vec3* out) {
    if (address == 0) return false;
    __try {
        *out = *reinterpret_cast<Vec3*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool WriteVec3(uintptr_t address, const Vec3& value) {
    if (address == 0) return false;
    __try {
        *reinterpret_cast<Vec3*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace core
