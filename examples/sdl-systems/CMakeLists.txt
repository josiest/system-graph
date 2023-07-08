cmake_minimum_required(VERSION 3.18)
project(sdl-system-example)

add_executable(sketch src/sketch.cpp)
set_target_properties(sketch PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE)

target_include_directories(sketch PRIVATE ../../include include)

find_package(SDL2 REQUIRED)
find_package(EnTT REQUIRED)
target_link_libraries(sketch PRIVATE SDL2 EnTT::EnTT)
