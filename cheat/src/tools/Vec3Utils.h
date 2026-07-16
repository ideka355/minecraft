#pragma once

#include <cstdint>
#include <vector>

#include "tools/IncrementalScanner.h"

namespace tools {

struct Vec3Match {
    uintptr_t address;  // x component; y at +4, z at +8
    float x, y, z;
};

// Finds candidates that have live companions at +4 and +8 -- i.e. look like a
// Vec3<float>. Results are sorted by proximity to `nearAddress` (closest first) when
// nonzero, since the real position/velocity Vec3 is expected near the player object.
std::vector<Vec3Match> FindVec3Triplets(const std::vector<ScanCandidate>& candidates,
                                         uintptr_t nearAddress = 0);

}  // namespace tools
