#include "modules/movement/Flight.h"

#include <windows.h>

#include "core/GameMemory.h"
#include "core/Offsets.h"

namespace modules {

namespace {
constexpr float kFlightSpeed = 0.6f;
}

Flight::Flight() : Module("Flight", VK_F1) {}

void Flight::OnTick() {
    const core::Offsets& offsets = core::GetOffsets();
    if (offsets.velocityOffset == 0) {
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

    if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        velocity.y = kFlightSpeed;
    } else if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
        velocity.y = -kFlightSpeed;
    } else {
        velocity.y = 0.f;
    }

    core::WriteVec3(player + offsets.velocityOffset, velocity);
}

}  // namespace modules
