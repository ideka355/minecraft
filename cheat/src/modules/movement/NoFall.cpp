#include "modules/movement/NoFall.h"

#include "core/GameMemory.h"
#include "core/Offsets.h"

namespace modules {

namespace {
constexpr float kFallSpeedThreshold = -0.5f;
}

NoFall::NoFall() : Module("NoFall", 0) {}  // no default keybind, toggle from the GUI

void NoFall::OnTick() {
    const core::Offsets& offsets = core::GetOffsets();
    if (offsets.velocityOffset == 0 || offsets.onGroundOffset == 0) {
        return;
    }

    uintptr_t player = core::GetLocalPlayer();
    if (!player) {
        return;
    }

    core::Vec3 velocity{};
    if (!core::ReadVec3(player + offsets.velocityOffset, &velocity)) {
        return;
    }

    if (velocity.y < kFallSpeedThreshold) {
        core::WriteBool(player + offsets.onGroundOffset, true);
    }
}

}  // namespace modules
