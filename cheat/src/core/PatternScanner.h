#pragma once

#include <cstdint>
#include <string>

namespace core {

// IDA-style pattern scanning ("48 8B 05 ? ? ? ? 48 8B 88") over a module's image.
// This is the tool you use, alongside a disassembler, to (re)locate the offsets in
// config/offsets.json every time Minecraft.Windows.exe updates.
class PatternScanner {
   public:
    // Scans the .text-containing range of `moduleBase` for `pattern`. Returns nullptr if
    // not found. `moduleBase` defaults to the main executable module of the current process.
    static uintptr_t Find(const std::string& pattern, uintptr_t moduleBase = 0);

   private:
    static uintptr_t FindInRange(const std::string& pattern, uint8_t* begin, size_t size);
};

}  // namespace core
