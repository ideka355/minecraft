add_executable(injector
    src/main.cpp
)

target_compile_definitions(injector PRIVATE UNICODE _UNICODE)
target_link_libraries(injector PRIVATE Shlwapi)
