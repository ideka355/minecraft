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
// Two ways to run it, both opened with F5:
//  - Manual (the "Start" button): you move/jump/type coordinates when prompted. You are
//    the correctness check at two points -- confirming position matches your F3
//    coordinates, and confirming a pointer chain survives a respawn.
//  - Auto Setup (the "Auto Setup" button): simulates WASD/jump via SendInput and drives
//    the whole flow unattended, substituting automatic heuristics for those two manual
//    checks (converge-until-a-unique-candidate-survives; pick the shortest surviving
//    pointer chain instead of respawn-validating). Faster and hands-off, but with neither
//    manual correctness check, it's more likely to occasionally lock onto wrong offsets
//    -- RenderStageDone() flags it if any auto-pick was ambiguous.
class OffsetWizard : public core::Module {
   public:
    OffsetWizard();

   protected:
    void OnEnable() override;
    void OnDisable() override;
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

    // Auto Setup: simulates the same moves/clicks a human would make. Called at the top of
    // OnTick() before Render() whenever autoMode_ is set; paces itself against real time
    // (GetTickCount64()) rather than frame count so it doesn't spam input every frame, and
    // never blocks -- all waiting is "not yet time to act, do nothing this frame".
    void DriveAutoMode();
    void StartAutoTap(int vk, int durationMs);
    // Non-blocking: releases the held key once its duration has elapsed and returns true
    // (exactly once, on that frame) so the caller knows a full tap just completed.
    bool UpdateAutoTap();
    void AutoDriveNarrowPosition();
    void AutoDriveNarrowVelocity();
    void AutoDriveNarrowOnGround();
    void PickShortestSurvivingChainAndFinish();

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

    // Auto Setup state. Timestamps are GetTickCount64() milliseconds.
    bool autoMode_ = false;
    bool autoAmbiguous_ = false;    // true if any auto-pick had to guess among multiple survivors
    bool autoChainRetried_ = false;
    int autoRoundCount_ = 0;
    int autoMoveKeyHeld_ = 0;       // VK code currently held down by auto-mode, 0 = none
    unsigned long long autoKeyReleaseTick_ = 0;
    unsigned long long autoNextActionTick_ = 0;
};

}  // namespace tools
