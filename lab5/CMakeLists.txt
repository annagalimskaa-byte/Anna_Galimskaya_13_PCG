cmake_minimum_required(VERSION 3.16)
project(lab5_clipping VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" "/usr/local/opt/qt" ${CMAKE_PREFIX_PATH})
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

add_library(clipping_core STATIC
    src/geometry.cpp
    src/clipping.cpp
    src/input_parser.cpp
)
target_include_directories(clipping_core PUBLIC include)

add_executable(lab5_clipping
    src/main.cpp
    src/main_window.cpp
    src/canvas_widget.cpp
    include/main_window.h
    include/canvas_widget.h
    resources/examples.qrc
)
target_include_directories(lab5_clipping PRIVATE include)
target_link_libraries(lab5_clipping PRIVATE clipping_core Qt6::Widgets Qt6::Core Qt6::Gui)
set_target_properties(lab5_clipping PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Lab5 Clipping"
    WIN32_EXECUTABLE TRUE
)

enable_testing()
add_executable(lab5_tests tests/unit_tests.cpp)
target_link_libraries(lab5_tests PRIVATE clipping_core)
add_test(NAME unit_tests COMMAND lab5_tests)
