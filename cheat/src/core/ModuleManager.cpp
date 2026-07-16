#include "core/ModuleManager.h"

namespace core {

ModuleManager& ModuleManager::Instance() {
    static ModuleManager instance;
    return instance;
}

void ModuleManager::Tick() {
    for (auto& module : modules_) {
        module->Tick();
    }
}

void ModuleManager::OnKeyDown(int vkCode) {
    for (auto& module : modules_) {
        if (module->Keybind() == vkCode) {
            module->Toggle();
        }
    }
}

}  // namespace core
