#pragma once

#include <string>

namespace core {

// Base class for every toggleable feature (Flight, Speed, ...). Subclasses override
// OnEnable/OnDisable for one-shot setup/teardown and OnTick for per-frame work.
class Module {
   public:
    Module(std::string name, int defaultKeybind) : name_(std::move(name)), keybind_(defaultKeybind) {}
    virtual ~Module() = default;

    const std::string& Name() const { return name_; }
    int Keybind() const { return keybind_; }
    bool Enabled() const { return enabled_; }

    void Toggle() {
        enabled_ = !enabled_;
        if (enabled_) {
            OnEnable();
        } else {
            OnDisable();
        }
    }

    void Tick() {
        if (enabled_) {
            OnTick();
        }
    }

   protected:
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnTick() {}

   private:
    std::string name_;
    int keybind_;
    bool enabled_ = false;
};

}  // namespace core
