#include "core/Offsets.h"

#include <fstream>
#include <sstream>

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

std::string ToHex(uintptr_t value) {
    std::ostringstream oss;
    oss << "0x" << std::hex << value;
    return oss.str();
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

bool Offsets::SaveToFile(const std::wstring& path) const {
    nlohmann::json j;
    j["_comment"] =
        "Written by the in-game offset auto-finder. Values are byte offsets/RVAs; hex "
        "strings are used for readability.";
    j["clientInstanceRva"] = ToHex(clientInstanceRva);
    j["positionOffset"] = ToHex(positionOffset);
    j["velocityOffset"] = ToHex(velocityOffset);
    j["onGroundOffset"] = ToHex(onGroundOffset);
    j["noClipFlagOffset"] = ToHex(noClipFlagOffset);

    nlohmann::json chain = nlohmann::json::array();
    for (uintptr_t hop : localPlayerChain) {
        chain.push_back(ToHex(hop));
    }
    j["localPlayerChain"] = chain;

    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    file << j.dump(2);
    return file.good();
}

Offsets& GetOffsets() {
    static Offsets offsets;
    return offsets;
}

std::wstring& GetPersistentConfigPath() {
    static std::wstring path;
    return path;
}

}  // namespace core
