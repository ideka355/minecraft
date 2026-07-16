#include "core/MemoryRegions.h"

#include <windows.h>

#include <algorithm>

namespace core {

std::vector<MemoryRegion> EnumerateWritableRegions(size_t maxTotalBytes) {
    std::vector<MemoryRegion> regions;

    uintptr_t address = 0x10000;  // skip the null-page guard region
    size_t totalBytes = 0;
    MEMORY_BASIC_INFORMATION mbi{};

    while (totalBytes < maxTotalBytes) {
        SIZE_T result =
            VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi));
        if (result != sizeof(mbi)) {
            break;
        }

        uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        size_t regionSize = mbi.RegionSize;

        bool writable = mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE &&
                         !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
                         (mbi.Protect &
                          (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE));

        if (writable && regionSize > 0) {
            size_t take = std::min(regionSize, maxTotalBytes - totalBytes);
            regions.push_back({regionBase, take});
            totalBytes += take;
        }

        uintptr_t next = regionBase + regionSize;
        if (next <= address) {
            break;  // overflow / stuck guard
        }
        address = next;
    }

    return regions;
}

bool IsReadableHeapPointer(uintptr_t address, const std::vector<MemoryRegion>& sortedRegions) {
    if (address < 0x10000) {
        return false;
    }

    auto it = std::upper_bound(
        sortedRegions.begin(), sortedRegions.end(), address,
        [](uintptr_t addr, const MemoryRegion& region) { return addr < region.base; });

    if (it == sortedRegions.begin()) {
        return false;
    }
    --it;
    return address >= it->base && address < it->base + it->size;
}

}  // namespace core
