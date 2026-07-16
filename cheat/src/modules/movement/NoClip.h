#pragma once

#include "core/Module.h"

namespace modules {

// Flips the player's collision flag off while enabled, restoring its original value on
// disable. Requires offsets.noClipFlagOffset to be set; this is very version-fragile
// (the flag may be part of a bitfield rather than a standalone bool in some builds).
class NoClip : public core::Module {
   public:
    NoClip();

   protected:
    void OnEnable() override;
    void OnDisable() override;
    void OnTick() override;

   private:
    bool originalValue_ = false;
    bool haveOriginalValue_ = false;
};

}  // namespace modules
