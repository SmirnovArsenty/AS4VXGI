cmake_minimum_required(VERSION 3.8)

project(as4vxgi)

### dependencies
add_subdirectory(third_party)
add_subdirectory(framework)

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
