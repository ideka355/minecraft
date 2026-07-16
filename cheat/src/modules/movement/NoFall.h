#pragma once

#include "core/Module.h"

namespace modules {

// Forces the "on ground" flag true whenever the player is falling fast enough that the
// next tick would register fall damage. This is a simplified heuristic (velocity
// threshold, not real collision) -- if your version tracks a separate fall-distance
// field, wire that in instead for accuracy.
class NoFall : public core::Module {
   public:
    NoFall();

   protected:
    void OnTick() override;
};

}  // namespace modules
