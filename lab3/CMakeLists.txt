cmake_minimum_required(VERSION 3.20)
project(Lab3_ImageProcessing VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

add_executable(Lab3_ImageProcessing
        src/main.cpp
        src/Image.cpp
        src/ImageProcessor.cpp
        src/MainWindow.cpp
        include/Image.h
        include/ImageProcessor.h
        include/MainWindow.h
)

target_include_directories(Lab3_ImageProcessing PRIVATE include)

target_link_libraries(Lab3_ImageProcessing PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Lab3_ImageProcessing PRIVATE -O3)
endif()

set_target_properties(Lab3_ImageProcessing PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Lab3_ImageProcessing"
)