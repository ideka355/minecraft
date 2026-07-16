#include "modules/movement/Speed.h"

#include <windows.h>

#include "core/GameMemory.h"
#include "core/Offsets.h"

namespace modules {

namespace {
constexpr float kMultiplier = 1.35f;
constexpr float kMaxHorizontalSpeed = 2.0f;  // safety clamp so a bad offset can't fling the player
}

Speed::Speed() : Module("Speed", VK_F2) {}

void Speed::OnTick() {
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

    velocity.x *= kMultiplier;
    velocity.z *= kMultiplier;

    auto clamp = [](float v) {
        if (v > kMaxHorizontalSpeed) return kMaxHorizontalSpeed;
        if (v < -kMaxHorizontalSpeed) return -kMaxHorizontalSpeed;
        return v;
    };
    velocity.x = clamp(velocity.x);
    velocity.z = clamp(velocity.z);

    core::WriteVec3(player + offsets.velocityOffset, velocity);
}

}  // namespace modules
