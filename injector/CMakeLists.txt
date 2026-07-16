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
# parallel with cheat's own still-in-progress object compiles. OBJECT_DEPENDS is meant to
# fix that, but Ninja doesn't reliably resolve a $<TARGET_FILE:...> generator expression
# given directly to it -- it ends up looking for a file literally named "$<TARGET_FILE:cheat>".
# Route through a plain-path stamp file instead: a real build edge (DEPENDS cheat) whose
# OUTPUT is an ordinary path, which OBJECT_DEPENDS handles fine.
set(CHEAT_DLL_STAMP "${CMAKE_CURRENT_BINARY_DIR}/cheat_dll.stamp")
add_custom_command(
    OUTPUT "${CHEAT_DLL_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E touch "${CHEAT_DLL_STAMP}"
    DEPENDS cheat
    VERBATIM
)
set_source_files_properties("${GENERATED_RC}" PROPERTIES OBJECT_DEPENDS "${CHEAT_DLL_STAMP}")

set_target_properties(injector PROPERTIES OUTPUT_NAME "BedrockCheat")

# NOMINMAX only -- WIN32_LEAN_AND_MEAN broke TRUSTEE_IS_WELLKNOWN_GROUP (accctrl.h's
# declarations end up incomplete under it even with an explicit #include <accctrl.h>) and
# the injector doesn't use std::min/max anyway, so there's nothing here for it to fix.
target_compile_definitions(injector PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(injector PRIVATE Shlwapi)
