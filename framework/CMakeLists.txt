cmake_minimum_required(VERSION 3.8)

project(framework)

### dependencies

### end of dependencies

set(group_core
    core/game.cpp
    core/game.h
)

set(group_render
    render/render.cpp
    render/render.h

    render/camera.cpp
    render/camera.h
)

set(group_render_resource
    render/resource/buffer.hpp
    render/resource/pipeline.cpp
    render/resource/pipeline.h
    render/resource/texture.cpp
    render/resource/texture.h
)

set(group_render_scene
    # render/scene/light.h
    # render/scene/material.cpp
    # render/scene/material.h
    # render/scene/mesh.cpp
    # render/scene/mesh.h
    # render/scene/model.cpp
    # render/scene/model.h
    # render/scene/scene.cpp
    # render/scene/scene.h
)

set(group_render_scene_lights
    render/scene/lights/ambient.cpp
    render/scene/lights/direction.cpp
    render/scene/lights/point.cpp
)

set(group_component_common
    component/game_component.h
)

set(group_win32
    win32/input.cpp
    win32/input.h
    win32/win.cpp
    win32/win.h
)

set(sources
    ${group_core}

    ${group_render}
    ${group_render_resource}
    ${group_render_scene}
    ${group_render_scene_lights}
    ${group_render_utils}

    ${group_component_common}

    ${group_component_impl}

    ${group_win32}
)

source_group("core" FILES ${group_core})
source_group("render" FILES ${group_render})
source_group("render/resource" FILES ${group_render_resource})
source_group("render/scene" FILES ${group_render_scene})
source_group("render/scene/lights" FILES ${group_render_scene_lights})
source_group("render/utils" FILES ${group_render_utils})
source_group("component/common" FILES ${group_component_common})
source_group("win32" FILES ${group_win32})

add_library(framework STATIC ${sources})
set_target_properties(framework PROPERTIES CXX_STANDARD 17)
target_link_libraries(framework
    assimp
    directxtk
    imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/render/lib/WinPixEventRuntime.lib
)
target_include_directories(framework
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. # to include shaders
)
add_custom_command(TARGET framework POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:assimp> ${CMAKE_CURRENT_SOURCE_DIR}/..)
