cmake_minimum_required(VERSION 3.8)

set(CMAKE_SYSTEM_VERSION "10.0.22621.0")

project(as4vxgi)

### dependencies
add_subdirectory(third_party)
add_subdirectory(framework)

set(fx_shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/resources/shaders/common/translate.fx
    ${CMAKE_CURRENT_SOURCE_DIR}/framework/resources/shaders/common/types.fx
)
set(vertex_shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/debug/box.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/debug/normal.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/voxels/draw.hlsl
)
set(geometry_shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/voxels/draw.hlsl
)
set(pixel_shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/debug/box.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/debug/normal.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/voxels/draw.hlsl
)
set(compute_shaders
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders/voxels/fill.hlsl
)

set(as4vxgi_math
    src/math/model_tree.cpp
    src/math/model_tree.h
)
source_group("math" FILES ${as4vxgi_math})

set(as4vxgi_main
    src/main.cpp
    src/as4vxgi.cpp
    src/as4vxgi.h
)
source_group("" FILES ${as4vxgi_main})

set(as4vxgi_sources
    ${as4vxgi_main}
    ${as4vxgi_math}
)
add_executable(as4vxgi WIN32 ${as4vxgi_sources})
set_target_properties(as4vxgi PROPERTIES CXX_STANDARD 17)
target_include_directories(as4vxgi
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/framework)
target_link_libraries(as4vxgi
    framework
)
set_property(TARGET as4vxgi PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_command(TARGET as4vxgi POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:as4vxgi> ${CMAKE_CURRENT_SOURCE_DIR})

set(dxc "C:/Program Files (x86)/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/dxc.exe")

add_custom_target(shaders ALL
    DEPENDS ${fx_shaders}
    DEPENDS ${compute_shaders}
    DEPENDS ${vertex_shaders}
    DEPENDS ${geometry_shaders}
    DEPENDS ${pixel_shaders}
)

foreach(shader IN LISTS compute_shaders)
    add_custom_command(TARGET shaders PRE_BUILD
        COMMAND "${dxc}" -E "CSMain" -T "cs_6_0" -Fo "${shader}.compute.cso" -Zi -Fd "${shader}.compute.pdb" -I "${CMAKE_CURRENT_SOURCE_DIR}/framework/shaders/common/" "${shader}"
        COMMAND echo ${shader}.compute
        VERBATIM)
endforeach()

foreach(shader IN LISTS vertex_shaders)
    add_custom_command(TARGET shaders PRE_BUILD
        COMMAND "${dxc}" -E "VSMain" -T "vs_6_0" -Fo "${shader}.vertex.cso" -Zi -Fd "${shader}.vertex.pdb" -I "${CMAKE_CURRENT_SOURCE_DIR}/framework/shaders/common/" "${shader}"
        COMMAND echo ${shader}.vertex
        VERBATIM)
endforeach()

foreach(shader IN LISTS geometry_shaders)
    add_custom_command(TARGET shaders PRE_BUILD
        COMMAND "${dxc}" -E "GSMain" -T "gs_6_0" -Fo "${shader}.geometry.cso" -Zi -Fd "${shader}.geometry.pdb" -I "${CMAKE_CURRENT_SOURCE_DIR}/framework/shaders/common/" "${shader}"
        COMMAND echo ${shader}.geometry
        VERBATIM)
endforeach()

foreach(shader IN LISTS pixel_shaders)
    add_custom_command(TARGET shaders PRE_BUILD
        COMMAND "${dxc}" -E "PSMain" -T "ps_6_0" -Fo "${shader}.pixel.cso" -Zi -Fd "${shader}.pixel.pdb" -I "${CMAKE_CURRENT_SOURCE_DIR}/framework/shaders/common/" "${shader}"
        COMMAND echo ${shader}.pixel
        VERBATIM)
endforeach()
