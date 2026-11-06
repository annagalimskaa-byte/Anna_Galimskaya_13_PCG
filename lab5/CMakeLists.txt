cmake_minimum_required(VERSION 3.20)
project(Lab5_Clipping VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" ${CMAKE_PREFIX_PATH})
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

add_executable(Lab5_Clipping
        src/main.cpp
        src/Clipper.cpp
        src/Canvas.cpp
        src/MainWindow.cpp
        include/Clipper.h
        include/Canvas.h
        include/MainWindow.h
)

target_include_directories(Lab5_Clipping PRIVATE include)

target_link_libraries(Lab5_Clipping PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
)
