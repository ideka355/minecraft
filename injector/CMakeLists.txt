# cheat.dll and the default offsets.json are embedded as resources so the whole project
# ships as a single injector executable. The generated .rc references cheat's build output
# via a generator expression, so cheat must build first (see add_dependencies below).
set(GENERATED_RC "${CMAKE_CURRENT_BINARY_DIR}/embedded.rc")

file(GENERATE OUTPUT "${GENERATED_RC}" CONTENT
"CHEATDLL RCDATA \"$<TARGET_FILE:cheat>\"\nOFFSETSDEFAULT RCDATA \"${CMAKE_SOURCE_DIR}/config/offsets.json\"\n"
)

add_executable(injector
    src/main.cpp
    "${GENERATED_RC}"
)

add_dependencies(injector cheat)

set_target_properties(injector PROPERTIES OUTPUT_NAME "BedrockCheat")

target_compile_definitions(injector PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(injector PRIVATE Shlwapi)
