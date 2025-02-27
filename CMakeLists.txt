cmake_minimum_required(VERSION 3.15...3.31)
project(ScratchPad)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG "release-3.2.4"
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui")

file(GLOB_RECURSE ScratchPadSources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

file(GLOB_RECURSE IMGUI_SOURCES CONFIGURE_DEPENDS
    "${IMGUI_DIR}/*.cpp"
    "${IMGUI_DIR}/*.h"
)

add_executable(ScratchPad ${ScratchPadSources})
target_sources(ScratchPad PRIVATE ${IMGUI_SOURCES})
target_include_directories(ScratchPad PUBLIC 
    ${IMGUI_DIR}
)
target_link_libraries(ScratchPad PUBLIC SDL3::SDL3)