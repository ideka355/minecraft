#pragma once

namespace tools {

// SendInput-based synthetic key press helpers, used by the offset auto-finder's "Auto
// Setup" mode to simulate the WASD/jump movement it needs to observe, instead of requiring
// the player to move by hand. Uses virtual-key codes (standard Win32 VK_* constants).
class InputSimulator {
   public:
    // Brings the game window to the foreground so synthetic input reaches it rather than
    // whatever else might have focus (this loader's own console, in particular).
    static void FocusGameWindow();

    static void KeyDown(int vk);
    static void KeyUp(int vk);
};

}  // namespace tools
