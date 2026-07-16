#pragma once

// Forward-declared rather than pulling in <windows.h> here.
struct HWND__;
using HWND = HWND__*;

namespace gui {

// Hooks the game's DXGI swapchain Present() to draw an ImGui overlay on top of the
// existing D3D11 render target, and hooks WndProc to feed input into ImGui while the
// click-GUI is open. This is a generic "external overlay via swapchain hook" -- it does
// not depend on any Bedrock-version-specific offsets.
class Overlay {
   public:
    // Locates the game's swapchain vtable (via a throwaway device/swapchain) and installs
    // the Present/ResizeBuffers hooks. Call once from Cheat::Init.
    static bool Install();
    static void Uninstall();
};

// The game's window handle, resolved once the swapchain hook first fires. nullptr before
// then. Used by tools::InputSimulator to focus the game before simulating key input.
HWND GetGameWindow();

}  // namespace gui
