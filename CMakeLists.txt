cmake_minimum_required(VERSION 3.10)
project(influxdb C)
set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Source files
set(SOURCES
    src/ifdb.c
)

# Create the static library
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Fix the output name to avoid "liblib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME influxdb)


# Link against cjson (private, only for the library itself)
target_link_libraries(${PROJECT_NAME} PRIVATE cjson)

# Installation rules for the library
install(TARGETS ${PROJECT_NAME}
    EXPORT libinfluxdbTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Installation rules for the headers
install(FILES ${CMAKE_SOURCE_DIR}/lib/libifdb.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export the targets to a CMake configuration
install(EXPORT libinfluxdbTargets
    FILE libinfluxdbTargets.cmake
    NAMESPACE libinfluxdb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libinfluxdb
)

# Add a package configuration file
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libinfluxdbConfigVersion.cmake"
    VERSION 1.0
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libinfluxdbConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libinfluxdbConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libinfluxdb
)

# Install the configuration files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libinfluxdbConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libinfluxdbConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libinfluxdb
)

