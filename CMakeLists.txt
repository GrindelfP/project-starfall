cmake_minimum_required(VERSION 3.24)
project(Starfall VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Starfall requires Windows x64 and MSVC.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_executable(starfall src/main.cpp src/dx11.cpp src/dx11.h)
target_link_libraries(starfall PRIVATE d3d11 dxgi)
target_compile_definitions(starfall PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)


