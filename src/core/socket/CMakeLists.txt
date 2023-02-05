cmake_minimum_required(VERSION 3.18)

set(CORE_SOCKET_CPP PhysicalSocket.cpp PhysicalSocketOption.cpp
                    SocketConnection.cpp SocketContext.cpp
)

set(CORE_SOCKET_H PhysicalSocket.h PhysicalSocketOption.h SocketConnection.h
                  SocketContext.h
)

add_library(core-socket SHARED ${CORE_SOCKET_CPP} ${CORE_SOCKET_H})
add_library(snodec::core-socket ALIAS core-socket)

target_include_directories(
    core-socket
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    core-socket PUBLIC snodec::core snodec::mux-${IO_Multiplexer}
                       snodec::logger snodec::utils
)

set_target_properties(
    core-socket PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-core-socket
)

install(
    TARGETS core-socket
    EXPORT snodec_core-socket_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "." EXCLUDE
    PATTERN "stream" EXCLUDE
    PATTERN "dgram" EXCLUDE
)

install(
    EXPORT snodec_core-socket_Targets
    FILE snodec_core-socket_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(stream)
