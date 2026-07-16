#include "tools/IncrementalScanner.h"

#include <windows.h>

#include <cmath>

#include "core/MemoryRegions.h"

namespace tools {

namespace {

constexpr float kMaxCoordMagnitude = 3.0e7f;  // Bedrock's world border is roughly this

size_t Stride(ScanValueType type) { return type == ScanValueType::Float ? 4 : 1; }

bool SafeRead(uintptr_t address, ScanValueType type, double* out) {
    __try {
        if (type == ScanValueType::Float) {
            float v = *reinterpret_cast<float*>(address);
            if (!std::isfinite(v) || std::fabs(v) > kMaxCoordMagnitude) {
                return false;
            }
            *out = static_cast<double>(v);
        } else {
            *out = static_cast<double>(*reinterpret_cast<uint8_t*>(address));
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

void IncrementalScanner::SeedRange(uintptr_t start, size_t size) {
    candidates_.clear();
    size_t stride = Stride(type_);
    for (uintptr_t addr = start; addr + stride <= start + size; addr += stride) {
        double v;
        if (Read(addr, &v)) {
            candidates_.push_back({addr, v});
        }
    }
}

void IncrementalScanner::SeedProcess(size_t maxTotalBytes) {
    candidates_.clear();
    size_t stride = Stride(type_);
    for (const auto& region : core::EnumerateWritableRegions(maxTotalBytes)) {
        for (uintptr_t addr = region.base; addr + stride <= region.base + region.size;
             addr += stride) {
            double v;
            if (Read(addr, &v)) {
                candidates_.push_back({addr, v});
            }
        }
    }
}

void IncrementalScanner::Narrow(double minDelta, double maxDelta) {
    std::vector<ScanCandidate> survivors;
    survivors.reserve(candidates_.size());

    for (const auto& c : candidates_) {
        double v;
        if (!Read(c.address, &v)) {
            continue;
        }
        double delta = std::fabs(v - c.lastValue);
        if (delta >= minDelta && delta <= maxDelta) {
            survivors.push_back({c.address, v});
        }
    }

    candidates_ = std::move(survivors);
}

bool IncrementalScanner::Read(uintptr_t address, double* out) const {
    return SafeRead(address, type_, out);
}

}  // namespace tools
