#include "core/PatternScanner.h"

#include <windows.h>

#include <optional>
#include <sstream>
#include <vector>

namespace core {

namespace {

struct PatternByte {
    uint8_t value;
    bool wildcard;
};

std::vector<PatternByte> ParsePattern(const std::string& pattern) {
    std::vector<PatternByte> bytes;
    std::istringstream stream(pattern);
    std::string token;

    while (stream >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back({0, true});
        } else {
            bytes.push_back({static_cast<uint8_t>(std::stoul(token, nullptr, 16)), false});
        }
    }

    return bytes;
}

}  // namespace

uintptr_t PatternScanner::FindInRange(const std::string& pattern, uint8_t* begin, size_t size) {
    const auto needle = ParsePattern(pattern);
    if (needle.empty() || size < needle.size()) {
        return 0;
    }

    for (size_t i = 0; i <= size - needle.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < needle.size(); ++j) {
            if (!needle[j].wildcard && begin[i + j] != needle[j].value) {
                match = false;
                break;
            }
        }
        if (match) {
            return reinterpret_cast<uintptr_t>(begin + i);
        }
    }

    return 0;
}

uintptr_t PatternScanner::Find(const std::string& pattern, uintptr_t moduleBase) {
    uint8_t* base = reinterpret_cast<uint8_t*>(
        moduleBase ? moduleBase : reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)));
    if (!base) {
        return 0;
    }

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto* ntHeaders =
        reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);
    size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;

    return FindInRange(pattern, base, imageSize);
}

}  // namespace core
