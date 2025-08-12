CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(modules)

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/modules)

FUNCTION(register_module module_name)
    FILE(GLOB_RECURSE SRC_FILES_CXX "${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/src/*.cpp")

    ADD_LIBRARY(${module_name} MODULE ${SRC_FILES_CXX})
    SET_TARGET_PROPERTIES(${module_name} PROPERTIES PREFIX "")

    TARGET_LINK_LIBRARIES(${module_name} PRIVATE icxxabi klibcpp libbus)
    TARGET_INCLUDE_DIRECTORIES(${module_name}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/include
    )
ENDFUNCTION()

register_module(test)