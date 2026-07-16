cmake_minimum_required(VERSION 3.21)
project(minecraft-bedrock-cheat LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows only (Minecraft Bedrock for Windows + DLL injection).")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(injector)
add_subdirectory(cheat)
