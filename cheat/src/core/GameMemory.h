#pragma once

#include <cstdint>

namespace core {

struct Vec3 {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

// Resolves the local player's object address by walking core::GetOffsets().localPlayerChain
// starting from the module base + clientInstanceRva. Returns 0 if offsets are unset or any
// hop in the chain reads a null pointer.
uintptr_t GetLocalPlayer();

// SEH-guarded read/write for plain-old-data types (float, bool, Vec3, ...). Guards against
// crashing the game when an offset is wrong or the target object has been freed. Defined
// only for the POD types modules actually use (see GameMemory.cpp).
bool ReadFloat(uintptr_t address, float* out);
bool WriteFloat(uintptr_t address, float value);
bool ReadBool(uintptr_t address, bool* out);
bool WriteBool(uintptr_t address, bool value);
bool ReadVec3(uintptr_t address, Vec3* out);
bool WriteVec3(uintptr_t address, const Vec3& value);

}  // namespace core
