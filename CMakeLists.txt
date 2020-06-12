cmake_minimum_required(VERSION 2.6)

set(solution_name EmuEZ)
project(${solution_name})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)





function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()
    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    get_all_targets(_targets "${_folder}")
    foreach(_target IN LISTS _targets)
        set_target_properties(
            ${_target}
            PROPERTIES FOLDER "${_folder_name}"
        )
    endforeach()
endfunction()




add_subdirectory_with_folder("Third Party/SDL2" ThirdParty/SDL2)


include_directories(ThirdParty/SDL2/include)


message("\nCompiling Utilities")
#Utilities
add_subdirectory_with_folder("Utilities" Code/Util)


message("\nCompiling Emulators")
#Emulators
#add_subdirectory(Code/Emulators/Base)
add_subdirectory_with_folder("Emulators" Code/Emulators/GB)


message("\nCompiling Platforms")
#Platforms
add_subdirectory(Code/Platforms/Win)



message("\n")