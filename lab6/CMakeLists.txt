cmake_minimum_required(VERSION 3.16)
project(lab6_3d_part VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(APPLE)
    set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt" "/usr/local/opt/qt" ${CMAKE_PREFIX_PATH})
endif()

add_library(part_core STATIC
    src/math3d.cpp
    src/mesh.cpp
    src/renderer.cpp
)
target_include_directories(part_core PUBLIC include)

enable_testing()
add_executable(lab6_tests tests/unit_tests.cpp)
target_link_libraries(lab6_tests PRIVATE part_core)
add_test(NAME unit_tests COMMAND lab6_tests)

add_executable(lab6_render_demo tools/render_demo.cpp)
target_link_libraries(lab6_render_demo PRIVATE part_core)

find_package(Qt6 COMPONENTS Widgets Core Gui)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    add_executable(lab6_3d_part
        src/main.cpp
        src/main_window.cpp
        src/viewport_widget.cpp
        include/main_window.h
        include/viewport_widget.h
        include/scene.h
    )
    target_include_directories(lab6_3d_part PRIVATE include)
    target_link_libraries(lab6_3d_part PRIVATE part_core Qt6::Widgets Qt6::Core Qt6::Gui)
    set_target_properties(lab6_3d_part PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Lab6 3D Part"
        WIN32_EXECUTABLE TRUE
    )
else()
    message(WARNING "Qt 6 не найден: собираются только ядро, тесты и lab6_render_demo. "
                    "Укажите путь к Qt через -DCMAKE_PREFIX_PATH=...")
endif()
