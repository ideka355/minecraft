#include "gui/ClickGui.h"

#include <imgui.h>
#include <windows.h>

#include "core/ModuleManager.h"

namespace gui {

bool ClickGui::visible = true;

void ClickGui::Render() {
    static bool wasInsertDown = false;
    bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
    if (insertDown && !wasInsertDown) {
        visible = !visible;
    }
    wasInsertDown = insertDown;

    if (!visible) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(280, 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("cheat");

    ImGui::TextDisabled("Insert to hide - module hotkeys toggle in-game");
    ImGui::Separator();

    for (auto& module : core::ModuleManager::Instance().Modules()) {
        bool enabled = module->Enabled();
        if (ImGui::Checkbox(module->Name().c_str(), &enabled)) {
            module->Toggle();
        }
    }

    ImGui::End();
}

}  // namespace gui
