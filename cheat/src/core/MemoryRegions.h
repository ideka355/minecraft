#pragma once

#include <cstdint>
#include <vector>

namespace core {

struct MemoryRegion {
    uintptr_t base;
    size_t size;
};

// Enumerates committed, writable, non-guard PRIVATE memory regions of the current process
// -- i.e. heap-like memory, as opposed to mapped module images. This is where dynamically
// allocated game objects (like the local player) live, and is the search space for the
// offset auto-finder's memory scans. Stops once `maxTotalBytes` worth of regions have been
// collected, to bound scan time.
std::vector<MemoryRegion> EnumerateWritableRegions(size_t maxTotalBytes);

// True if `address` falls inside one of the process's committed, readable regions --
// i.e. it's plausible to treat a value as a pointer and dereference it. Built from the
// same enumeration as above.
bool IsReadableHeapPointer(uintptr_t address, const std::vector<MemoryRegion>& sortedRegions);

}  // namespace core
