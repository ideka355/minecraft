#include "tools/Vec3Utils.h"

#include <windows.h>

#include <algorithm>
#include <unordered_set>

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

}  // namespace

std::vector<Vec3Match> FindVec3Triplets(const std::vector<ScanCandidate>& candidates,
                                         uintptr_t nearAddress) {
    std::unordered_set<uintptr_t> addrs;
    addrs.reserve(candidates.size() * 2);
    for (const auto& c : candidates) {
        addrs.insert(c.address);
    }

    std::vector<Vec3Match> result;
    for (const auto& c : candidates) {
        if (!addrs.count(c.address + 4) || !addrs.count(c.address + 8)) {
            continue;
        }
        float x, y, z;
        if (SafeReadFloat(c.address, &x) && SafeReadFloat(c.address + 4, &y) &&
            SafeReadFloat(c.address + 8, &z)) {
            result.push_back({c.address, x, y, z});
        }
    }

    if (nearAddress != 0) {
        std::sort(result.begin(), result.end(), [nearAddress](const Vec3Match& a, const Vec3Match& b) {
            auto dist = [nearAddress](uintptr_t addr) {
                return addr > nearAddress ? addr - nearAddress : nearAddress - addr;
            };
            return dist(a.address) < dist(b.address);
        });
    }

    return result;
}

}  // namespace tools
