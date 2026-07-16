#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "core/Module.h"
#include "tools/IncrementalScanner.h"
#include "tools/PointerScanner.h"
#include "tools/Vec3Utils.h"

namespace tools {

// In-game guided offset finder. Walks the user through locating position, velocity, and
// on-ground in memory via incremental value scanning (the same technique Cheat Engine
// uses), then searches for a pointer chain from the module's static data back to the
// found position and lets the user validate it survives a respawn. Writes the result to
// offsets.json when done.
//
// This can't run unattended: finding "which memory address is position" requires you to
// actually move so the scanner can see what changes. Open the panel with F5.
class OffsetWizard : public core::Module {
   public:
    OffsetWizard();

   protected:
    void OnEnable() override;
    void OnTick() override;

   private:
    enum class Stage {
        Intro,
        ScanningPositionSeed,   // background thread
        NarrowPosition,
        ConfirmPosition,
        NarrowVelocity,
        ConfirmVelocity,
        NarrowOnGround,
        ConfirmOnGround,
        ReadyForChainScan,
        ScanningChain,          // background thread
        ChainFailed,
        ValidateChain,
        Done,
    };

    void Render();
    void RenderStageIntro();
    void RenderStageScanningPositionSeed();
    void RenderStageNarrowPosition();
    void RenderStageConfirmPosition();
    void RenderStageNarrowVelocity();
    void RenderStageConfirmVelocity();
    void RenderStageNarrowOnGround();
    void RenderStageConfirmOnGround();
    void RenderStageReadyForChainScan();
    void RenderStageScanningChain();
    void RenderStageChainFailed();
    void RenderStageValidateChain();
    void RenderStageDone();

    void StartPositionScan();
    void StartChainScan();
    void Reset();
    void SaveResult();

    Stage stage_ = Stage::Intro;

    IncrementalScanner positionScanner_{ScanValueType::Float};
    IncrementalScanner velocityScanner_{ScanValueType::Float};
    IncrementalScanner onGroundScanner_{ScanValueType::Byte};

    std::vector<Vec3Match> positionCandidates_;
    std::vector<Vec3Match> velocityCandidates_;
    std::vector<ScanCandidate> onGroundCandidates_;
    std::vector<PointerChain> chainCandidates_;
    std::vector<bool> chainStillValid_;
    PointerChain chosenChain_;

    uintptr_t positionAddress_ = 0;
    uintptr_t velocityAddress_ = 0;
    uintptr_t onGroundAddress_ = 0;

    float validateInput_[3] = {0.f, 0.f, 0.f};

    std::thread worker_;
    std::atomic<bool> busy_ = false;
};

}  // namespace tools
