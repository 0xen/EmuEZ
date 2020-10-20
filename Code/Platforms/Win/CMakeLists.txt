cmake_minimum_required(VERSION 2.6)

set(platform_name Win)
set(project_name ${solution_name}-${platform_name})
message("\nBuilding: ${project_name}")
message(${CMAKE_HOME_DIRECTORY}/../../../Build)
message(${CMAKE_HOME_DIRECTORY}/../../../Build)
project(${project_name})


set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    ${CMAKE_HOME_DIRECTORY}/EmuEZ
    )
set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    ${CMAKE_HOME_DIRECTORY}/EmuEZ
    )


set(src
    src/main.cpp
    src/Renderer.cpp
    src/UI.cpp
    src/Window.cpp
    src/Visualisation.cpp
    src/Core.cpp
    src/EmulationManager.cpp
)
 
set(headers
    include/Renderer.hpp
    include/UI.hpp
    include/Window.hpp
    include/Visualisation.hpp
    include/Core.hpp
    include/EmulationManager.hpp
)

# Load imgui
set(imgui_src
    ../../../ThirdParty/imgui-docking/imgui.cpp
    ../../../ThirdParty/imgui-docking/imgui_demo.cpp
    ../../../ThirdParty/imgui-docking/imgui_draw.cpp
    ../../../ThirdParty/imgui-docking/imgui_widgets.cpp
)
set(imgui_headers
    ../../../ThirdParty/imgui-docking/imconfig.h
    ../../../ThirdParty/imgui-docking/imgui.h
    ../../../ThirdParty/imgui-docking/imgui_internal.h
)
list(APPEND src ${imgui_src})
list(APPEND headers ${imgui_headers})

source_group("Source Files\\External\\imgui" FILES ${imgui_src})
source_group("Header Files\\External\\imgui" FILES ${imgui_headers})

include_directories(include)

include_directories(../../../../Vulkan/Vk-Helper/include)

include_directories(../../../ThirdParty/imgui-docking)
include_directories(../../../ThirdParty/glew-2.1.0/include/)

include_directories(../../Emulators/Base/include/)
include_directories(../../Emulators/GB/include/)

include_directories(../../Util/include)

add_executable(${project_name} ${src} ${headers})

set_property(TARGET ${project_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/EmuEZ")

target_link_libraries(${project_name} ${solution_name}-GB)
target_link_libraries(${project_name} ${solution_name}-Util)

target_link_libraries(${project_name} SDL2main)
target_link_libraries(${project_name} SDL2-static)

target_compile_features(${project_name} PRIVATE cxx_std_17)



target_link_libraries( ${project_name} debug ../../../../../Vulkan/build/Vk-Helper/Debug/Vk-Helper)
target_link_libraries( ${project_name} optimized ../../../../../Vulkan/build/Vk-Helper/Release/Vk-Helper)

find_package(Vulkan)

if(Vulkan_FOUND)
	target_include_directories(${project_name} PRIVATE Vulkan::Vulkan)
	target_link_libraries(${project_name} Vulkan::Vulkan)
endif()