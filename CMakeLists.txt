cmake_minimum_required(VERSION 3.24)
project(Starfall VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Starfall requires Windows x64 and MSVC.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_executable(starfall WIN32 src/main.cpp src/dx11.cpp src/dx11.h)
target_link_options(starfall PRIVATE /ENTRY:mainCRTStartup)
target_link_libraries(starfall PRIVATE d3d11 dxgi d3dcompiler)
target_compile_definitions(starfall PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.hlsl"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.hlsl" "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.hlsl"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.hlsl" VERBATIM)
add_custom_target(starfall_shaders ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.hlsl")
add_dependencies(starfall starfall_shaders)

enable_testing()
add_executable(starfall_input_timing_tests tests/input_timing_tests.cpp)
target_include_directories(starfall_input_timing_tests PRIVATE src)
target_compile_definitions(starfall_input_timing_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME starfall_input_timing_tests COMMAND starfall_input_timing_tests)

