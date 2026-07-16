#pragma once

#include <cstdint>
#include <vector>

namespace tools {

enum class ScanValueType { Float, Byte };

struct ScanCandidate {
    uintptr_t address;
    double lastValue;
};

// Generic Cheat-Engine-style incremental scanner: seed a set of addresses with their
// current value, then repeatedly narrow to addresses whose value changed in a way that
// matches a delta window. Drives the position/velocity/on-ground auto-finder: position is
// seeded across all scannable process memory, velocity/on-ground are seeded from a small
// window around the already-found position address.
class IncrementalScanner {
   public:
    explicit IncrementalScanner(ScanValueType type) : type_(type) {}

    // Seeds candidates from every aligned address in [start, start + size).
    void SeedRange(uintptr_t start, size_t size);

    // Seeds candidates from every aligned address across scannable process memory, capped
    // at maxTotalBytes worth of regions. Slow (can take several seconds) -- call from a
    // background thread.
    void SeedProcess(size_t maxTotalBytes);

    // Keeps only candidates whose value changed since the last read by an amount within
    // [minDelta, maxDelta].
    void Narrow(double minDelta, double maxDelta);

    size_t CandidateCount() const { return candidates_.size(); }
    const std::vector<ScanCandidate>& Candidates() const { return candidates_; }

   private:
    ScanValueType type_;
    std::vector<ScanCandidate> candidates_;

    bool Read(uintptr_t address, double* out) const;
};

}  // namespace tools
