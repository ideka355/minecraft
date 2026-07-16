#pragma once

#include <cstdint>

namespace core {

// Thin wrapper around MinHook so callers don't touch the library directly.
class Hook {
   public:
    static bool Init();
    static void Shutdown();

    // Creates and enables a hook on `target`, redirecting to `detour`.
    // On success, `*original` receives a trampoline that calls the original function.
    static bool Attach(void* target, void* detour, void** original);
    static bool Detach(void* target);
};

}  // namespace core
