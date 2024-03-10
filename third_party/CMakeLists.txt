cmake_minimum_required(VERSION 3.8)

project(third_party)

# directxtk
set(directxtk_sources
    # simple math
    ${CMAKE_CURRENT_SOURCE_DIR}/directxtk/Inc/SimpleMath.h
    ${CMAKE_CURRENT_SOURCE_DIR}/directxtk/Inc/SimpleMath.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/directxtk/Src/SimpleMath.cpp

    # texture loader
    ${CMAKE_CURRENT_SOURCE_DIR}/directxtk/Src/WICTextureLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/directxtk/Inc/WICTextureLoader.h
)
add_library(directxtk STATIC ${directxtk_sources})
target_include_directories(directxtk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/directxtk/Inc)
target_precompile_headers(directxtk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/directxtk/Src/pch.h)

# imgui
set(imgui_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_dx12.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_dx12.h
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_win32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_win32.h
)
add_library(imgui STATIC ${imgui_sources})
target_include_directories(imgui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# assimp
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/assimp)
