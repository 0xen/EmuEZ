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
)
 
set(headers
)

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

IF (WIN32)
## Warnings as errors
target_compile_options(${project_name} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)
ENDIF()