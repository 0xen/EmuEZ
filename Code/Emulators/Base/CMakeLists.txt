cmake_minimum_required(VERSION 2.6)

set(emu_name_base Base)
set(project_name ${solution_name}-${emu_name_base})
message("\nBuilding: ${project_name}")
message(${CMAKE_HOME_DIRECTORY}/../../../Build/bin/Emulators/${emu_name_base})
message(${CMAKE_HOME_DIRECTORY}/../../../Build/bin-debug/Emulators/${emu_name_base})
project(${project_name})

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    ${CMAKE_HOME_DIRECTORY}/bin-debug/Emulators/${emu_name_base}
    )
set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    ${CMAKE_HOME_DIRECTORY}/bin/Emulators/${emu_name_base}
    )


set(src
)
 
set(headers
    include/Base.hpp
)

add_library(${project_name} STATIC ${src} ${headers})