cmake_minimum_required(VERSION 2.6)

set(platform_name Util)
set(project_name ${solution_name}-${platform_name})
message("\nBuilding: ${project_name}")
message(${CMAKE_HOME_DIRECTORY}/../../../Build/bin/${platform_name})
message(${CMAKE_HOME_DIRECTORY}/../../../Build/bin-debug/${platform_name})
project(${project_name})


set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    ${CMAKE_HOME_DIRECTORY}/bin-debug/${platform_name}
    )
set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    ${CMAKE_HOME_DIRECTORY}/bin/${platform_name}
    )


set(src
    src/IO.cpp
)
 
set(headers
    include/Definitions.hpp
    include/IO.hpp
)

include_directories(include)

set(source
    ${src}
)

set(headers
    ${headers}
)

add_library(${project_name} STATIC ${src} ${headers})

## Warnings as errors
target_compile_options(${project_name} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)