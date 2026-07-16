find_package(minhook CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_library(cheat SHARED
    src/dllmain.cpp
    src/Cheat.cpp
    src/core/Hook.cpp
    src/core/PatternScanner.cpp
    src/core/Offsets.cpp
    src/core/GameMemory.cpp
    src/core/ModuleManager.cpp
    src/core/InputManager.cpp
    src/core/MemoryRegions.cpp
    src/gui/Overlay.cpp
    src/gui/ClickGui.cpp
    src/modules/movement/Flight.cpp
    src/modules/movement/Speed.cpp
    src/modules/movement/NoFall.cpp
    src/modules/movement/NoClip.cpp
    src/modules/movement/Spider.cpp
    src/tools/IncrementalScanner.cpp
    src/tools/Vec3Utils.cpp
    src/tools/PointerScanner.cpp
    src/tools/OffsetWizard.cpp
)

set_target_properties(cheat PROPERTIES OUTPUT_NAME "cheat")

target_include_directories(cheat PRIVATE src)
target_compile_definitions(cheat PRIVATE UNICODE _UNICODE)

target_link_libraries(cheat PRIVATE
    minhook::minhook
    nlohmann_json::nlohmann_json
    imgui::imgui
    d3d11
    dxgi
)
