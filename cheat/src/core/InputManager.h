#pragma once

namespace core {

// Polls keybinds once per frame (called from the render hook) and forwards edge-triggered
// key-down events to ModuleManager. Polling (vs. a WndProc hook) keeps this from fighting
// the game's own input handling or the ImGui overlay's input capture.
class InputManager {
   public:
    static void PollAndDispatch();
};

}  // namespace core
