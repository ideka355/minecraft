#pragma once

#include "core/Module.h"

namespace modules {

// Scales the local player's horizontal velocity by a fixed multiplier each tick,
// preserving the direction the game's own movement code already computed.
class Speed : public core::Module {
   public:
    Speed();

   protected:
    void OnTick() override;
};

}  // namespace modules
