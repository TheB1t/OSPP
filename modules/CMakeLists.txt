CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(modules)

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -r -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/modules)

FUNCTION(register_module module_name)
    FILE(GLOB_RECURSE SRC_FILES_CXX "${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/src/*.cpp")

    ADD_LIBRARY(${module_name}_objs OBJECT ${SRC_FILES_CXX})
    TARGET_INCLUDE_DIRECTORIES(${module_name}_objs
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/include
            $<TARGET_PROPERTY:klibcpp,INTERFACE_INCLUDE_DIRECTORIES>
    )

    SEPARATE_ARGUMENTS(SPLIT_LINKER_FLAGS UNIX_COMMAND "${CMAKE_LINKER_FLAGS}")
    ADD_CUSTOM_COMMAND(
        OUTPUT  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${module_name}.ko
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        COMMAND ${CMAKE_LINKER} ${SPLIT_LINKER_FLAGS} -r
                -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${module_name}.ko
                $<TARGET_OBJECTS:${module_name}_objs>
                --whole-archive $<TARGET_FILE:klibcpp> --no-whole-archive
        DEPENDS ${SRC_FILES_CXX} ${module_name}_objs klibcpp
        VERBATIM
    )

    ADD_CUSTOM_TARGET(${module_name} ALL
        DEPENDS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${module_name}.ko
    )
ENDFUNCTION()

register_module(test)