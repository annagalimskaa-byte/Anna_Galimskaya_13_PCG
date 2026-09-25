cmake_minimum_required(VERSION 3.20)
project(ImageParserLab VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ImageParserLab
        src/main.cpp
        src/BmpParser.cpp
        src/PngParser.cpp
        src/JpegParser.cpp
        src/GifParser.cpp
        src/TiffParser.cpp
        src/PcxParser.cpp
        src/ThreadPool.cpp
)

target_include_directories(ImageParserLab PRIVATE include)

find_package(Threads REQUIRED)
target_link_libraries(ImageParserLab PRIVATE Threads::Threads)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(ImageParserLab PRIVATE -O3)
endif()