cmake_minimum_required(VERSION 3.0)

pkg_check_modules(BLUEZ REQUIRED bluez)

add_library(
    netbt SHARED address/L2CapAddress.cpp address/RfCommAddress.cpp
                 l2cap/Socket.cpp rfcomm/Socket.cpp
)
add_library(snodec::netbt ALIAS netbt)

target_link_libraries(
    netbt PUBLIC snodec::logger snodec::utils snodec::net ${BLUEZ_LIBRARIES}
)

set_target_properties(
    netbt
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_include_directories(
    netbt
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${BLUEZ_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

install(
    TARGETS netbt
    EXPORT snodecnetbtTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/socket/bluetooth
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnetbtTargets
    FILE snodecnetbtTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
