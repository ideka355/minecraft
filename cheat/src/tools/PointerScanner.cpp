#include "tools/PointerScanner.h"

#include <windows.h>

#include <unordered_map>

#include "core/MemoryRegions.h"

namespace tools {

namespace {

constexpr size_t kMaxFrontier = 20000;
constexpr uintptr_t kOffsetStep = 8;
constexpr size_t kMaxResults = 200;

bool SafeReadPtr(uintptr_t address, uintptr_t* out) {
    __try {
        *out = *reinterpret_cast<uintptr_t*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

struct FrontierNode {
    uintptr_t rva;
    std::vector<uintptr_t> hops;
    uintptr_t address;
};

// Keeps at most one path per resolved address (first one found), to stop the frontier
// from re-exploring the same intermediate object via many equivalent static pointers.
std::vector<FrontierNode> Dedupe(std::vector<FrontierNode> nodes) {
    std::unordered_map<uintptr_t, size_t> seen;
    std::vector<FrontierNode> result;
    for (auto& node : nodes) {
        if (seen.emplace(node.address, result.size()).second) {
            result.push_back(std::move(node));
        }
    }
    if (result.size() > kMaxFrontier) {
        result.resize(kMaxFrontier);
    }
    return result;
}

}  // namespace

std::vector<PointerChain> PointerScanner::Find(uintptr_t target, int maxDepth,
                                                uintptr_t maxOffset) {
    std::vector<PointerChain> results;

    uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(moduleBase);
    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(moduleBase + dosHeader->e_lfanew);
    size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;

    auto regions = core::EnumerateWritableRegions(1536ull * 1024 * 1024);

    // Level 0 (roots): every 8-byte slot in the module's own image that holds a plausible
    // heap pointer.
    std::vector<FrontierNode> frontier;
    for (uintptr_t slot = moduleBase; slot + 8 <= moduleBase + imageSize; slot += 8) {
        uintptr_t value;
        if (!SafeReadPtr(slot, &value)) {
            continue;
        }
        uintptr_t rva = slot - moduleBase;
        if (value == target) {
            results.push_back({rva, {}});
        } else if (core::IsReadableHeapPointer(value, regions)) {
            frontier.push_back({rva, {}, value});
        }
    }

    frontier = Dedupe(std::move(frontier));

    // Levels 1..maxDepth-1: walk outward from the frontier by a small offset and
    // dereference, looking for `target` or another plausible pointer to keep exploring.
    for (int depth = 1; depth < maxDepth && !frontier.empty() && results.size() < kMaxResults;
         ++depth) {
        std::vector<FrontierNode> next;
        bool canGoDeeper = depth < maxDepth - 1;

        for (const auto& node : frontier) {
            for (uintptr_t offset = 0; offset <= maxOffset; offset += kOffsetStep) {
                uintptr_t value;
                if (!SafeReadPtr(node.address + offset, &value)) {
                    continue;
                }

                if (value == target) {
                    std::vector<uintptr_t> hops = node.hops;
                    hops.push_back(offset);
                    results.push_back({node.rva, std::move(hops)});
                    if (results.size() >= kMaxResults) {
                        break;
                    }
                } else if (canGoDeeper && core::IsReadableHeapPointer(value, regions)) {
                    std::vector<uintptr_t> hops = node.hops;
                    hops.push_back(offset);
                    next.push_back({node.rva, std::move(hops), value});
                }
            }
            if (results.size() >= kMaxResults) {
                break;
            }
        }

        frontier = Dedupe(std::move(next));
    }

    return results;
}

bool PointerScanner::Resolve(const PointerChain& chain, uintptr_t* outAddress) {
    uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    uintptr_t address = moduleBase + chain.rva;

    if (!SafeReadPtr(address, &address)) {
        return false;
    }
    for (uintptr_t hop : chain.hops) {
        if (!SafeReadPtr(address + hop, &address)) {
            return false;
        }
    }

    *outAddress = address;
    return true;
}

}  // namespace tools
