cmake_minimum_required(VERSION 3.21)

if(NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "This project targets Windows only (Minecraft Bedrock for Windows + DLL injection).")
endif()

project(minecraft-bedrock-cheat LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# cheat must come first: injector embeds its output as a resource.
add_subdirectory(cheat)
add_subdirectory(injector)
