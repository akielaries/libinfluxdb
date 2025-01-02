cmake_minimum_required(VERSION 3.10)
project(libinfluxdb C)
set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)

include_directories(${CMAKE_SOURCE_DIR}/lib)

set(SOURCES
    src/ifdb.c
)

# create the static library
add_library(${PROJECT_NAME} STATIC ${SOURCES})

install(TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR} 
)

install(FILES ${CMAKE_SOURCE_DIR}/lib/libifdb.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Set a custom prefix for installation (e.g., /usr/local or /opt/libinfluxdb)
# Default is /usr/local
# set(CMAKE_INSTALL_PREFIX "/path/to/your/directory")

