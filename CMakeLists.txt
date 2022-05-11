cmake_minimum_required(VERSION 3.1.0)

set(PROJECT_NAME table_generator)

project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif ()

find_package(Qt5 COMPONENTS Core REQUIRED)

add_executable(${PROJECT_NAME}
        main.cpp
        latex.h)

target_link_libraries(${PROJECT_NAME} Qt5::Core)
