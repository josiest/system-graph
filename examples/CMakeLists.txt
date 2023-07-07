cmake_minimum_required(VERSION 3.18)
project(subsystem-interface-example)

add_executable(sketch sketch.cpp)
target_include_directories(sketch PRIVATE ../include)

set_target_properties(sketch PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE)

find_package(EnTT REQUIRED)
target_link_libraries(sketch PRIVATE EnTT::EnTT)
