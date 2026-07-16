#include "modules/movement/NoClip.h"

#include <windows.h>

#include "core/GameMemory.h"
#include "core/Offsets.h"

namespace modules {

NoClip::NoClip() : Module("NoClip", VK_F3) {}

void NoClip::OnEnable() {
    const core::Offsets& offsets = core::GetOffsets();
    uintptr_t player = core::GetLocalPlayer();
    if (!player || offsets.noClipFlagOffset == 0) {
        return;
    }
    haveOriginalValue_ =
        core::ReadBool(player + offsets.noClipFlagOffset, &originalValue_);
}

void NoClip::OnDisable() {
    if (!haveOriginalValue_) {
        return;
    }
    const core::Offsets& offsets = core::GetOffsets();
    uintptr_t player = core::GetLocalPlayer();
    if (player && offsets.noClipFlagOffset != 0) {
        core::WriteBool(player + offsets.noClipFlagOffset, originalValue_);
    }
    haveOriginalValue_ = false;
}

void NoClip::OnTick() {
    const core::Offsets& offsets = core::GetOffsets();
    if (offsets.noClipFlagOffset == 0) {
        return;
    }
    uintptr_t player = core::GetLocalPlayer();
    if (!player) {
        return;
    }
    core::WriteBool(player + offsets.noClipFlagOffset, true);
}

}  // namespace modules
