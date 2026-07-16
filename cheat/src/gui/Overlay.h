#pragma once

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

}  // namespace gui
