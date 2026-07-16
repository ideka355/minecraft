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

# add_dependencies() alone only orders injector's final link after cheat -- it doesn't stop
# Ninja from compiling embedded.rc (which needs cheat.dll's bytes to actually exist) in
# parallel with cheat's own still-in-progress object compiles. OBJECT_DEPENDS adds a real
# file-level dependency so the RC compile step waits for the DLL to exist first.
set_source_files_properties("${GENERATED_RC}" PROPERTIES OBJECT_DEPENDS "$<TARGET_FILE:cheat>")

set_target_properties(injector PROPERTIES OUTPUT_NAME "BedrockCheat")

target_compile_definitions(injector PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(injector PRIVATE Shlwapi)
