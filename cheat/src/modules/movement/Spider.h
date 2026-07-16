#pragma once

#include "core/Module.h"

namespace modules {

// Lets you climb by holding a movement key, approximating "spider" wall-climb. This is a
// simplification: it applies upward velocity on movement input alone, without checking
// for an actual wall collision, since that requires a collision-side offset this scaffold
// doesn't have. If your version exposes one, gate OnTick() on it for a real implementation.
class Spider : public core::Module {
   public:
    Spider();

   protected:
    void OnTick() override;
};

}  // namespace modules
