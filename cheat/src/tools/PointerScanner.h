#pragma once

#include <cstdint>
#include <vector>

namespace tools {

struct PointerChain {
    uintptr_t rva = 0;             // offset from the module base to the root static slot
    std::vector<uintptr_t> hops;   // offsets applied+dereferenced after the root hop
};

// Breadth-first search for a chain of pointers, rooted in the main module's static image
// data, that resolves to `target`. This is the automated version of what Cheat Engine's
// "pointer scan" does: walk outward from every plausible static pointer, following
// dereference+small-offset hops, looking for a path that lands on the target address.
// Expensive (can take tens of seconds) -- run from a background thread.
class PointerScanner {
   public:
    // maxDepth: total number of dereferences considered, including the root hop (so
    // maxDepth=3 means "root -> +hop -> +hop -> target", i.e. up to 2 entries in hops).
    // maxOffset: largest per-hop byte offset considered; struct member offsets rarely
    // exceed a few KB.
    static std::vector<PointerChain> Find(uintptr_t target, int maxDepth = 3,
                                           uintptr_t maxOffset = 0x1000);

    // Re-resolves a previously found chain against the current process state. Used to
    // check whether a candidate chain still lands on a fresh target address after a
    // respawn/relog -- the standard way to reject chains that matched by coincidence.
    static bool Resolve(const PointerChain& chain, uintptr_t* outAddress);
};

}  // namespace tools
