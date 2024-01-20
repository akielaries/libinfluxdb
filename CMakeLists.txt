cmake_minimum_required(VERSION 3.00)

project(libinfluxdb VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# source files
set(SOURCES
    src/ #TODO.c
)

# include dir
set(INCLUDE_DIRS
    libinfluxdb
)

# create static lib
add_library(libinfluxdb STATIC ${SOURCES})

# include directories for the library
target_include_directories(libinfluxdb PUBLIC ${INCLUDE_DIRS})

# version of the library
set_target_properties(libinfluxdb PROPERTIES VERSION ${PROJECT_VERSION})

# install the library
include(GNUInstallDirs)
install(TARGETS libinfluxdb
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libinfluxdb
)

# install the headers
install(FILES libinfluxdb/libifdb.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libinfluxdb)

