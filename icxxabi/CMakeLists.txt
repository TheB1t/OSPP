CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(icxxabi)

add_library(${PROJECT_NAME} STATIC src/icxxabi.cpp)
add_library(crti OBJECT src/crti.c)
add_library(crtn OBJECT src/crtn.c)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)