#pragma once

#include "core/Module.h"

namespace modules {

// Hovers by overwriting the local player's vertical velocity every tick, holding
// horizontal (x/z) velocity as the game's own movement code set it. Space = up,
// Left Shift = down, neither = hover in place.
class Flight : public core::Module {
   public:
    Flight();

   protected:
    void OnTick() override;
};

}  // namespace modules
