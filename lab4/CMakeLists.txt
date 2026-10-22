cmake_minimum_required(VERSION 3.20)
project(Lab4_RasterAlgorithms VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

add_executable(Lab4_RasterAlgorithms
        src/main.cpp
        src/Rasterizer.cpp
        src/Canvas.cpp
        src/MainWindow.cpp
        include/Rasterizer.h
        include/Canvas.h
        include/MainWindow.h
)

target_include_directories(Lab4_RasterAlgorithms PRIVATE include)

target_link_libraries(Lab4_RasterAlgorithms PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Lab4_RasterAlgorithms PRIVATE -O3)
endif()

set_target_properties(Lab4_RasterAlgorithms PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Lab4_RasterAlgorithms"
)