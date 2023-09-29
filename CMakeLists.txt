cmake_minimum_required(VERSION 3.8)

project(as4vxgi)

### dependencies
add_subdirectory(third_party)
add_subdirectory(framework)

set(as4vxgi_sources
    src/main.cpp
)
source_group("" FILES ${as4vxgi_sources})
add_executable(as4vxgi WIN32 ${as4vxgi_sources})
target_include_directories(as4vxgi
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/framework)
target_link_libraries(as4vxgi
    framework
)
set_property(TARGET as4vxgi PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_command(TARGET as4vxgi POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:as4vxgi> ${CMAKE_CURRENT_SOURCE_DIR})
