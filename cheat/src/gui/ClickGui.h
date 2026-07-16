#pragma once

namespace gui {

// Renders the ImGui window listing every registered module with an enable/disable
// checkbox. Toggled on/off with Insert. Called from Overlay's hooked Present, after
// ImGui::NewFrame().
class ClickGui {
   public:
    static void Render();
    static bool visible;
};

}  // namespace gui
