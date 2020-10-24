cmake_minimum_required(VERSION 2.6)

set(emu_name_gb PSX)
set(project_name ${solution_name}-${emu_name_gb})
message("\nBuilding: ${project_name}")
message(${CMAKE_HOME_DIRECTORY}/../../../Build/bin/Emulators/${emu_name_gb})
message(${CMAKE_HOME_DIRECTORY}/../../../Build/bin-debug/Emulators/${emu_name_gb})
project(${project_name})

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    ${CMAKE_HOME_DIRECTORY}/bin-debug/Emulators/${emu_name_gb}
    )
set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    ${CMAKE_HOME_DIRECTORY}/bin/Emulators/${emu_name_gb}
    )


set(src
    src/PSX.cpp
)
 
set(headers
    include/PSX.hpp
)

include_directories(include)
include_directories(../Base/include)
include_directories(../../Util/include)

set(source
    ${src}
)

set(headers
    ${headers}
)

add_library(${project_name} STATIC ${src} ${headers})

target_link_libraries( ${project_name} ${solution_name}-Util)
target_include_directories(${project_name} PRIVATE ../Base/include/)

target_compile_features(${project_name} PRIVATE cxx_std_17)


#IF (WIN32)
### Warnings as errors
#target_compile_options(${project_name} PRIVATE
#  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
#  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
#)
#ENDIF()