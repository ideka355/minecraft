#include "core/InputManager.h"

#include <windows.h>

#include <unordered_map>

#include "core/ModuleManager.h"

namespace core {

void InputManager::PollAndDispatch() {
    static std::unordered_map<int, bool> wasDown;

    for (auto& module : ModuleManager::Instance().Modules()) {
        int vk = module->Keybind();
        if (vk == 0) {
            continue;
        }

        bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (isDown && !wasDown[vk]) {
            ModuleManager::Instance().OnKeyDown(vk);
        }
        wasDown[vk] = isDown;
    }
}

}  // namespace core
