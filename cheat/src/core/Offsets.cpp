#include "core/Offsets.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace core {

namespace {

uintptr_t HexField(const nlohmann::json& j, const char* key) {
    if (!j.contains(key)) {
        return 0;
    }
    const auto& v = j.at(key);
    if (v.is_string()) {
        return static_cast<uintptr_t>(std::stoull(v.get<std::string>(), nullptr, 16));
    }
    if (v.is_number_unsigned()) {
        return static_cast<uintptr_t>(v.get<uint64_t>());
    }
    return 0;
}

}  // namespace

bool Offsets::LoadFromFile(const std::wstring& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }

    clientInstanceRva = HexField(j, "clientInstanceRva");
    positionOffset = HexField(j, "positionOffset");
    velocityOffset = HexField(j, "velocityOffset");
    onGroundOffset = HexField(j, "onGroundOffset");
    noClipFlagOffset = HexField(j, "noClipFlagOffset");

    if (j.contains("localPlayerChain") && j.at("localPlayerChain").is_array()) {
        const auto& chain = j.at("localPlayerChain");
        for (size_t i = 0; i < chain.size() && i < std::size(localPlayerChain); ++i) {
            const auto& v = chain[i];
            localPlayerChain[i] = v.is_string()
                                       ? static_cast<uintptr_t>(std::stoull(
                                             v.get<std::string>(), nullptr, 16))
                                       : static_cast<uintptr_t>(v.get<uint64_t>());
        }
    }

    return true;
}

Offsets& GetOffsets() {
    static Offsets offsets;
    return offsets;
}

}  // namespace core
