#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core/Module.h"

namespace core {

// Owns every module instance and drives keybind handling + the per-frame tick.
class ModuleManager {
   public:
    static ModuleManager& Instance();

    template <typename T, typename... Args>
    T* Register(Args&&... args) {
        auto module = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = module.get();
        modules_.push_back(std::move(module));
        return raw;
    }

    const std::vector<std::unique_ptr<Module>>& Modules() const { return modules_; }

    // Call once per frame from the render hook.
    void Tick();

    // Call from the WndProc hook (or a raw input poll) on key-down events.
    void OnKeyDown(int vkCode);

   private:
    std::vector<std::unique_ptr<Module>> modules_;
};

}  // namespace core
