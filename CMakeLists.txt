cmake_minimum_required(VERSION 3.24)
project(Starfall VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Starfall requires Windows x64 and MSVC.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



add_executable(starfall WIN32 src/main.cpp src/dx11.cpp src/dx11.h src/dx12.cpp src/dx12.h)
add_executable(starfall_dx11 WIN32 src/main.cpp src/dx11.cpp src/dx11.h src/dx12.cpp src/dx12.h)
add_executable(starfall_dx12 WIN32 src/main.cpp src/dx11.cpp src/dx11.h src/dx12.cpp src/dx12.h)
foreach(target IN ITEMS starfall starfall_dx11 starfall_dx12)
    target_link_options(${target} PRIVATE /ENTRY:mainCRTStartup)
    target_link_libraries(${target} PRIVATE d3d11 d3d12 dxgi dxguid d3dcompiler)
    target_compile_definitions(${target} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
endforeach()
target_compile_definitions(starfall_dx12 PRIVATE STARFALL_DEFAULT_DX12)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/scene.hlsl"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene.hlsl" "${CMAKE_CURRENT_BINARY_DIR}/shaders/scene.hlsl"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene.hlsl" VERBATIM)
add_custom_target(starfall_shaders ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/shaders/scene.hlsl")
add_dependencies(starfall starfall_shaders)
add_dependencies(starfall_dx11 starfall_shaders)
add_dependencies(starfall_dx12 starfall_shaders)

enable_testing()
add_executable(starfall_input_timing_tests tests/input_timing_tests.cpp)
target_include_directories(starfall_input_timing_tests PRIVATE src)
target_compile_definitions(starfall_input_timing_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_test(NAME starfall_input_timing_tests COMMAND starfall_input_timing_tests)


add_executable(starfall_scene_tests tests/scene_tests.cpp)
target_include_directories(starfall_scene_tests PRIVATE src)
add_test(NAME starfall_scene_tests COMMAND starfall_scene_tests)


if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/camera_tests.cpp")
    add_executable(starfall_camera_tests tests/camera_tests.cpp)
    target_include_directories(starfall_camera_tests PRIVATE src)
    add_test(NAME starfall_camera_tests COMMAND starfall_camera_tests)
endif()


find_program(STARFALL_DXC dxc HINTS "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64" REQUIRED)
foreach(stage IN ITEMS vs ps)
    if(stage STREQUAL "vs")
        set(entry VSMain)
    else()
        set(entry PSMain)
    endif()
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/scene12_${stage}.dxil"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND "${STARFALL_DXC}" -T ${stage}_6_0 -E ${entry} -Fo "${CMAKE_CURRENT_BINARY_DIR}/shaders/scene12_${stage}.dxil" "${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene12.hlsl"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/scene12.hlsl" VERBATIM)
    list(APPEND dx12_shaders "${CMAKE_CURRENT_BINARY_DIR}/shaders/scene12_${stage}.dxil")
endforeach()
add_custom_target(starfall_dx12_shaders ALL DEPENDS ${dx12_shaders})
add_dependencies(starfall starfall_dx12_shaders)
add_dependencies(starfall_dx11 starfall_dx12_shaders)
add_dependencies(starfall_dx12 starfall_dx12_shaders)

add_executable(starfall_ship_tests tests/ship_tests.cpp)
target_include_directories(starfall_ship_tests PRIVATE src)
add_test(NAME starfall_ship_tests COMMAND starfall_ship_tests)

add_executable(starfall_asteroid_tests tests/asteroid_tests.cpp)
target_include_directories(starfall_asteroid_tests PRIVATE src)
add_test(NAME starfall_asteroid_tests COMMAND starfall_asteroid_tests)
