#pragma once

namespace cheat {

// Top-level init/shutdown, called from DllMain on DLL_PROCESS_ATTACH/DETACH.
// Runs on the injected thread, not the render thread -- it only sets up hooks and
// registers modules, it never touches game memory directly.
void Init();
void Shutdown();

}  // namespace cheat
