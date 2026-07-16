#include "modules/movement/Spider.h"

#include <windows.h>

#include "core/GameMemory.h"
#include "core/Offsets.h"

namespace modules {

namespace {
constexpr float kClimbSpeed = 0.2f;
}

Spider::Spider() : Module("Spider", VK_F4) {}

void Spider::OnTick() {
    const core::Offsets& offsets = core::GetOffsets();
    if (offsets.velocityOffset == 0) {
        return;
    }

    bool movingIntoWall = (GetAsyncKeyState('W') & 0x8000) != 0;
    if (!movingIntoWall) {
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

    velocity.y = kClimbSpeed;
    core::WriteVec3(player + offsets.velocityOffset, velocity);
}

}  // namespace modules
