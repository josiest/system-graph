cmake_minimum_required(VERSION 3.18)
project(pi-systems VERSION 0.1.0)

#
# Define the libarary
#

include(GNUInstallDirs)
add_library(pi-systems INTERFACE)
add_library(pi::systems ALIAS pi-systems)
target_compile_features(pi-systems INTERFACE cxx_std_20)

find_package(EnTT REQUIRED)
target_link_libraries(pi-systems INTERFACE EnTT::EnTT)

target_include_directories(pi-systems INTERFACE
    $<BUILD_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_sources(pi-systems INTERFACE
    $<BUILD_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/pi/graphs/graphs.hpp>
    $<BUILD_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include/pi/systems/system_graph.hpp>)

#
# Export and Install library targets
#

install(TARGETS pi-systems
        EXPORT pi-systems-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

export(EXPORT pi-systems-targets
       FILE ${CMAKE_CURRENT_BINARY_DIR}/systems-targets.cmake
       NAMESPACE pi::)

install(EXPORT pi-systems-targets
        FILE systems-targets.cmake
        NAMESPACE pi::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pi)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#
# Create and Install package
#

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/systems-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/systems-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/systems-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pi)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/systems-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/systems-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pi)

export(PACKAGE pi-systems)
