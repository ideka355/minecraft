#pragma once

#include <cstdint>
#include <string>

namespace core {

// All game-specific addresses/offsets live here, loaded from config/offsets.json at
// attach time. NONE of these have real values checked in -- they are version-specific
// and only exist by reverse-engineering the exact Minecraft.Windows.exe build you're
// running (x64dbg/IDA/Ghidra + a pattern scan via PatternScanner, or cross-referencing a
// public offset dump for that version). Every module checks IsValid() before doing
// anything and refuses to run against zeroed-out offsets rather than write to garbage
// addresses.
struct Offsets {
    // RVA (relative to the module base) of a static pointer that, once dereferenced,
    // eventually leads to the local player's Actor/Player instance.
    uintptr_t clientInstanceRva = 0;

    // Pointer-chase offsets applied in order after reading *(base + clientInstanceRva).
    // Typically something like ClientInstance -> LocalPlayer* -> ... Populate as many
    // hops as your version's class layout needs; leave the rest at 0.
    uintptr_t localPlayerChain[4] = {0, 0, 0, 0};

    // Byte offsets *within* the resolved Player/Actor object.
    uintptr_t positionOffset = 0;   // Vec3<float> world position
    uintptr_t velocityOffset = 0;   // Vec3<float> velocity
    uintptr_t onGroundOffset = 0;   // bool (or flags bitfield) "is on ground"
    uintptr_t noClipFlagOffset = 0; // bool/flags controlling collision, if present

    // clientInstanceRva == 0 is used as the "not configured" sentinel -- a real static
    // pointer never legitimately lives at RVA 0 (that's the module's own PE header).
    // positionOffset == 0 IS a valid value (the offset finder frequently resolves the
    // pointer chain directly to the position field), so it isn't checked here.
    bool IsValid() const { return clientInstanceRva != 0; }

    // Loads from a JSON file (see config/offsets.json for the schema). Returns false
    // (and leaves everything zeroed) if the file is missing or malformed.
    bool LoadFromFile(const std::wstring& path);

    // Writes the current values back out in the same schema LoadFromFile reads. Used by
    // the offset auto-finder to persist what it discovers.
    bool SaveToFile(const std::wstring& path) const;
};

// Process-wide singleton, populated once during DllMain / Cheat::Init.
Offsets& GetOffsets();

// Path to the user-editable offsets.json next to the loader exe, as opposed to this DLL's
// own (temp-directory) copy. Set once at attach time from a marker file the injector
// leaves behind, so the offset auto-finder can write its results somewhere that survives
// past this run. Empty if unknown.
std::wstring& GetPersistentConfigPath();

}  // namespace core
