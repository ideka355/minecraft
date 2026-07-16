#include "tools/OffsetWizard.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include <imgui.h>

#include "core/Offsets.h"

namespace tools {

namespace {

bool SafeReadFloat(uintptr_t address, float* out) {
    __try {
        *out = *reinterpret_cast<float*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SafeReadByte(uintptr_t address, uint8_t* out) {
    __try {
        *out = *reinterpret_cast<uint8_t*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

constexpr uintptr_t kWindowRadius = 0x1000;  // search velocity/on-ground within +-4KB of position

}  // namespace

OffsetWizard::OffsetWizard() : Module("Offset Finder", VK_F5) {}

void OffsetWizard::OnEnable() {}

void OffsetWizard::OnTick() { Render(); }

void OffsetWizard::Render() {
    ImGui::SetNextWindowSize(ImVec2(440, 360), ImGuiCond_FirstUseEver);
    ImGui::Begin("Offset Finder");

    switch (stage_) {
        case Stage::Intro:
            RenderStageIntro();
            break;
        case Stage::ScanningPositionSeed:
            RenderStageScanningPositionSeed();
            break;
        case Stage::NarrowPosition:
            RenderStageNarrowPosition();
            break;
        case Stage::ConfirmPosition:
            RenderStageConfirmPosition();
            break;
        case Stage::NarrowVelocity:
            RenderStageNarrowVelocity();
            break;
        case Stage::ConfirmVelocity:
            RenderStageConfirmVelocity();
            break;
        case Stage::NarrowOnGround:
            RenderStageNarrowOnGround();
            break;
        case Stage::ConfirmOnGround:
            RenderStageConfirmOnGround();
            break;
        case Stage::ReadyForChainScan:
            RenderStageReadyForChainScan();
            break;
        case Stage::ScanningChain:
            RenderStageScanningChain();
            break;
        case Stage::ChainFailed:
            RenderStageChainFailed();
            break;
        case Stage::ValidateChain:
            RenderStageValidateChain();
            break;
        case Stage::Done:
            RenderStageDone();
            break;
    }

    ImGui::End();
}

void OffsetWizard::RenderStageIntro() {
    ImGui::TextWrapped(
        "Finds position/velocity/on-ground offsets by watching what changes in memory as "
        "you move -- it needs you to actually move when asked, it can't run unattended.");
    ImGui::Separator();
    ImGui::TextWrapped("Stand still somewhere safe in the world, then click Start.");
    if (ImGui::Button("Start")) {
        StartPositionScan();
    }
}

void OffsetWizard::StartPositionScan() {
    busy_ = true;
    stage_ = Stage::ScanningPositionSeed;
    worker_ = std::thread([this] {
        positionScanner_.SeedProcess(1536ull * 1024 * 1024);
        busy_ = false;
    });
}

void OffsetWizard::RenderStageScanningPositionSeed() {
    ImGui::TextWrapped("Scanning process memory... this can take up to a minute.");
    if (!busy_) {
        if (worker_.joinable()) {
            worker_.join();
        }
        stage_ = Stage::NarrowPosition;
    }
}

void OffsetWizard::RenderStageNarrowPosition() {
    ImGui::Text("%zu candidates remaining.", positionScanner_.CandidateCount());
    ImGui::TextWrapped(
        "Walk in a straight line for about a second, then click Narrow. Repeat 2-3 times "
        "until the count gets small.");

    if (ImGui::Button("Narrow (I just moved)")) {
        positionScanner_.Narrow(0.01, 8.0);
    }
    ImGui::SameLine();
    if (ImGui::Button("Find position candidates")) {
        positionCandidates_ = FindVec3Triplets(positionScanner_.Candidates());
        if (!positionCandidates_.empty()) {
            stage_ = Stage::ConfirmPosition;
        }
    }

    if (positionScanner_.CandidateCount() == 0) {
        ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f),
                            "No candidates left -- start over and try smaller/slower moves.");
        if (ImGui::Button("Start Over")) {
            Reset();
        }
    }
}

void OffsetWizard::RenderStageConfirmPosition() {
    ImGui::TextWrapped("Click the entry matching your F3 coordinates:");
    for (size_t i = 0; i < positionCandidates_.size() && i < 32; ++i) {
        const auto& m = positionCandidates_[i];
        float x = m.x, y = m.y, z = m.z;
        SafeReadFloat(m.address, &x);
        SafeReadFloat(m.address + 4, &y);
        SafeReadFloat(m.address + 8, &z);

        char label[128];
        std::snprintf(label, sizeof(label), "0x%p: %.2f, %.2f, %.2f",
                       reinterpret_cast<void*>(m.address), x, y, z);
        if (ImGui::Selectable(label)) {
            positionAddress_ = m.address;
            velocityScanner_.SeedRange(positionAddress_ - kWindowRadius, kWindowRadius * 2);
            stage_ = Stage::NarrowVelocity;
        }
    }
    if (ImGui::Button("Back")) {
        stage_ = Stage::NarrowPosition;
    }
}

void OffsetWizard::RenderStageNarrowVelocity() {
    ImGui::Text("%zu candidates remaining near position.", velocityScanner_.CandidateCount());
    ImGui::TextWrapped("Walk or strafe again, then click Narrow.");

    if (ImGui::Button("Narrow (I just moved)")) {
        velocityScanner_.Narrow(0.001, 4.0);
    }
    ImGui::SameLine();
    if (ImGui::Button("Find velocity candidates")) {
        auto found = FindVec3Triplets(velocityScanner_.Candidates(), positionAddress_);
        found.erase(std::remove_if(found.begin(), found.end(),
                                    [this](const Vec3Match& m) { return m.address == positionAddress_; }),
                    found.end());
        velocityCandidates_ = std::move(found);
        if (!velocityCandidates_.empty()) {
            stage_ = Stage::ConfirmVelocity;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Skip velocity")) {
        velocityAddress_ = 0;
        onGroundScanner_.SeedRange(positionAddress_ - kWindowRadius, kWindowRadius * 2);
        stage_ = Stage::NarrowOnGround;
    }
}

void OffsetWizard::RenderStageConfirmVelocity() {
    ImGui::TextWrapped("Click the entry that tracks your movement speed/direction:");
    for (size_t i = 0; i < velocityCandidates_.size() && i < 32; ++i) {
        const auto& m = velocityCandidates_[i];
        float x = m.x, y = m.y, z = m.z;
        SafeReadFloat(m.address, &x);
        SafeReadFloat(m.address + 4, &y);
        SafeReadFloat(m.address + 8, &z);

        char label[160];
        std::snprintf(label, sizeof(label), "0x%p (%+lld from position): %.3f, %.3f, %.3f",
                       reinterpret_cast<void*>(m.address),
                       static_cast<long long>(m.address) - static_cast<long long>(positionAddress_),
                       x, y, z);
        if (ImGui::Selectable(label)) {
            velocityAddress_ = m.address;
            onGroundScanner_.SeedRange(positionAddress_ - kWindowRadius, kWindowRadius * 2);
            stage_ = Stage::NarrowOnGround;
        }
    }
    if (ImGui::Button("Back")) {
        stage_ = Stage::NarrowVelocity;
    }
}

void OffsetWizard::RenderStageNarrowOnGround() {
    ImGui::Text("%zu candidates remaining near position.", onGroundScanner_.CandidateCount());
    ImGui::TextWrapped("Jump a few times (Space), landing between each, then click Narrow.");

    if (ImGui::Button("Narrow (I just jumped)")) {
        onGroundScanner_.Narrow(1.0, 1.0);
    }
    ImGui::SameLine();
    if (ImGui::Button("Find on-ground candidates")) {
        onGroundCandidates_ = onGroundScanner_.Candidates();
        std::sort(onGroundCandidates_.begin(), onGroundCandidates_.end(),
                  [this](const ScanCandidate& a, const ScanCandidate& b) {
                      auto dist = [this](uintptr_t addr) {
                          return addr > positionAddress_ ? addr - positionAddress_
                                                          : positionAddress_ - addr;
                      };
                      return dist(a.address) < dist(b.address);
                  });
        if (!onGroundCandidates_.empty()) {
            stage_ = Stage::ConfirmOnGround;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Skip on-ground")) {
        onGroundAddress_ = 0;
        stage_ = Stage::ReadyForChainScan;
    }
}

void OffsetWizard::RenderStageConfirmOnGround() {
    ImGui::TextWrapped("Click the entry that flips between 0 and 1 as you land/jump:");
    for (size_t i = 0; i < onGroundCandidates_.size() && i < 32; ++i) {
        const auto& c = onGroundCandidates_[i];
        uint8_t v = 0;
        SafeReadByte(c.address, &v);

        char label[128];
        std::snprintf(label, sizeof(label), "0x%p (%+lld from position): %d",
                       reinterpret_cast<void*>(c.address),
                       static_cast<long long>(c.address) - static_cast<long long>(positionAddress_),
                       static_cast<int>(v));
        if (ImGui::Selectable(label)) {
            onGroundAddress_ = c.address;
            stage_ = Stage::ReadyForChainScan;
        }
    }
    if (ImGui::Button("Back")) {
        stage_ = Stage::NarrowOnGround;
    }
}

void OffsetWizard::RenderStageReadyForChainScan() {
    ImGui::TextWrapped(
        "Last step: searching for a stable pointer chain from the game's static memory "
        "back to your position, so this keeps working across relaunches. Can take up to a "
        "couple of minutes.");
    if (ImGui::Button("Start Chain Scan")) {
        StartChainScan();
    }
}

void OffsetWizard::StartChainScan() {
    busy_ = true;
    stage_ = Stage::ScanningChain;
    worker_ = std::thread([this] {
        chainCandidates_ = PointerScanner::Find(positionAddress_, 3, 0x1000);
        busy_ = false;
    });
}

void OffsetWizard::RenderStageScanningChain() {
    ImGui::TextWrapped("Searching for a pointer chain... this can take a while.");
    if (!busy_) {
        if (worker_.joinable()) {
            worker_.join();
        }
        if (chainCandidates_.empty()) {
            stage_ = Stage::ChainFailed;
        } else if (chainCandidates_.size() == 1) {
            chosenChain_ = chainCandidates_.front();
            SaveResult();
            stage_ = Stage::Done;
        } else {
            chainStillValid_.assign(chainCandidates_.size(), true);
            stage_ = Stage::ValidateChain;
        }
    }
}

void OffsetWizard::RenderStageChainFailed() {
    ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "No pointer chain found.");
    ImGui::TextWrapped(
        "Try a deeper/wider search, or fall back to manual reverse engineering (x64dbg / "
        "Cheat Engine pointer scan) for this field.");
    if (ImGui::Button("Try a deeper search (slower)")) {
        busy_ = true;
        stage_ = Stage::ScanningChain;
        worker_ = std::thread([this] {
            chainCandidates_ = PointerScanner::Find(positionAddress_, 4, 0x4000);
            busy_ = false;
        });
    }
    if (ImGui::Button("Start Over")) {
        Reset();
    }
}

void OffsetWizard::RenderStageValidateChain() {
    ImGui::Text("%zu candidate chains found.", chainCandidates_.size());
    ImGui::TextColored(
        ImVec4(1.f, 0.8f, 0.2f, 1.f),
        "Important: die and respawn (or fully relog into the world) before validating -- "
        "otherwise nothing has changed and this check can't tell candidates apart.");
    ImGui::InputFloat3("Current position (F3)", validateInput_);

    if (ImGui::Button("Validate")) {
        int survivors = 0;
        int survivorIndex = -1;
        for (size_t i = 0; i < chainCandidates_.size(); ++i) {
            uintptr_t resolved = 0;
            bool ok = PointerScanner::Resolve(chainCandidates_[i], &resolved);
            float x = 0, y = 0, z = 0;
            if (ok && resolved) {
                ok = SafeReadFloat(resolved, &x) && SafeReadFloat(resolved + 4, &y) &&
                     SafeReadFloat(resolved + 8, &z);
            }
            bool matches = ok && std::fabs(x - validateInput_[0]) < 1.0f &&
                            std::fabs(y - validateInput_[1]) < 1.0f &&
                            std::fabs(z - validateInput_[2]) < 1.0f;
            chainStillValid_[i] = matches;
            if (matches) {
                ++survivors;
                survivorIndex = static_cast<int>(i);
            }
        }

        if (survivors == 1) {
            chosenChain_ = chainCandidates_[survivorIndex];
            SaveResult();
            stage_ = Stage::Done;
        }
    }

    int remaining = 0;
    for (bool v : chainStillValid_) {
        if (v) ++remaining;
    }
    ImGui::Text("%d / %zu chains still match.", remaining, chainCandidates_.size());
    if (remaining > 1) {
        ImGui::TextWrapped(
            "More than one still matches -- respawn again and re-validate to narrow "
            "further, or use the shortest one as a best guess.");
        if (ImGui::Button("Use shortest surviving chain")) {
            size_t bestIdx = 0;
            size_t bestLen = SIZE_MAX;
            for (size_t i = 0; i < chainCandidates_.size(); ++i) {
                if (chainStillValid_[i] && chainCandidates_[i].hops.size() < bestLen) {
                    bestLen = chainCandidates_[i].hops.size();
                    bestIdx = i;
                }
            }
            chosenChain_ = chainCandidates_[bestIdx];
            SaveResult();
            stage_ = Stage::Done;
        }
    } else if (remaining == 0) {
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
                            "None matched -- check the coordinates you typed and try again.");
    }
}

void OffsetWizard::RenderStageDone() {
    ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "Done -- offsets saved.");
    ImGui::TextWrapped(
        "Movement modules are now live. Close this window (F5) or click Reset to run the "
        "finder again (e.g. for a different offset).");
    if (ImGui::Button("Reset")) {
        Reset();
    }
}

void OffsetWizard::Reset() {
    stage_ = Stage::Intro;
    positionCandidates_.clear();
    velocityCandidates_.clear();
    onGroundCandidates_.clear();
    chainCandidates_.clear();
    chainStillValid_.clear();
    positionAddress_ = 0;
    velocityAddress_ = 0;
    onGroundAddress_ = 0;
    validateInput_[0] = validateInput_[1] = validateInput_[2] = 0.f;
}

void OffsetWizard::SaveResult() {
    core::Offsets& offsets = core::GetOffsets();
    offsets.clientInstanceRva = chosenChain_.rva;
    for (size_t i = 0; i < std::size(offsets.localPlayerChain); ++i) {
        offsets.localPlayerChain[i] = i < chosenChain_.hops.size() ? chosenChain_.hops[i] : 0;
    }
    offsets.positionOffset = 0;  // the chain resolves directly to the position field
    offsets.velocityOffset =
        velocityAddress_ ? velocityAddress_ - positionAddress_ : 0;
    offsets.onGroundOffset =
        onGroundAddress_ ? onGroundAddress_ - positionAddress_ : 0;

    const std::wstring& persistentPath = core::GetPersistentConfigPath();
    if (!persistentPath.empty() && offsets.SaveToFile(persistentPath)) {
        std::printf("Offset Finder: saved offsets.json\n");
    } else {
        std::printf(
            "Offset Finder: applied for this session, but couldn't save offsets.json to "
            "disk -- you'll need to re-run this next launch.\n");
    }
}

}  // namespace tools
