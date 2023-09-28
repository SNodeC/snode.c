cmake_minimum_required(VERSION 3.18)

set(NET_CPP
    PhysicalSocketOption.cpp
    config/ConfigInstance.cpp
    config/ConfigConnection.cpp
    config/ConfigLegacy.cpp
    config/ConfigListen.cpp
    config/ConfigPhysicalSocket.cpp
    config/ConfigPhysicalSocketClient.cpp
    config/ConfigPhysicalSocketServer.cpp
    config/ConfigSection.cpp
    config/ConfigTls.cpp
    config/ConfigTlsServer.cpp
    config/ConfigTlsClient.cpp
)

set(NET_H
    PhysicalSocket.h
    PhysicalSocket.hpp
    PhysicalSocketOption.h
    SocketAddress.h
    SocketAddress.hpp
    config/ConfigAddress.h
    config/ConfigAddress.hpp
    config/ConfigAddressLocal.h
    config/ConfigAddressLocal.hpp
    config/ConfigAddressRemote.h
    config/ConfigAddressRemote.hpp
    config/ConfigInstance.h
    config/ConfigConnection.h
    config/ConfigLegacy.h
    config/ConfigListen.h
    config/ConfigPhysicalSocket.h
    config/ConfigPhysicalSocketClient.h
    config/ConfigPhysicalSocketServer.h
    config/ConfigSection.h
    config/ConfigSection.hpp
    config/ConfigTls.h
    config/ConfigTlsServer.h
    config/ConfigTlsClient.h
    dgram/PeerSocket.h
    dgram/PeerSocket.hpp
    stream/PhysicalSocket.h
    stream/PhysicalSocket.hpp
    stream/PhysicalClientSocket.h
    stream/PhysicalClientSocket.hpp
    stream/PhysicalServerSocket.h
    stream/PhysicalServerSocket.hpp
    stream/config/ConfigSocketClient.h
    stream/config/ConfigSocketClient.hpp
    stream/config/ConfigSocketServer.h
    stream/config/ConfigSocketServer.hpp
    stream/config/legacy/ConfigSocketClient.h
    stream/config/legacy/ConfigSocketClient.hpp
    stream/config/legacy/ConfigSocketServer.h
    stream/config/legacy/ConfigSocketServer.hpp
    stream/config/tls/ConfigSocketClient.h
    stream/config/tls/ConfigSocketClient.hpp
    stream/config/tls/ConfigSocketServer.h
    stream/config/tls/ConfigSocketServer.hpp
)

set(SNODEC_DEFAULT_READBLOCKSIZE
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_READBLOCKSIZE},>,16384,${SNODEC_DEFAULT_READBLOCKSIZE}>
)
set(SNODEC_DEFAULT_WRITEBLOCKSIZE
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_WRITEBLOCKSIZE},>,16384,${SNODEC_DEFAULT_WRITEBLOCKSIZE}>
)
set(SNODEC_DEFAULT_READTIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_READTIMEOUT},>,60,${SNODEC_DEFAULT_READTIMEOUT}>
)
set(SNODEC_DEFAULT_WRITETIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_WRITETIMEOUT},>,60,${SNODEC_DEFAULT_WRITETIMEOUT}>
)
set(SNODEC_DEFAULT_TERMINATETIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_TERMINATETIMEOUT},>,1,${SNODEC_DEFAULT_TERMINATETIMEOUT}>
)
set_source_files_properties(
    config/ConfigConnection.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "SNODEC_DEFAULT_READBLOCKSIZE=${SNODEC_DEFAULT_READBLOCKSIZE};\
SNODEC_DEFAULT_WRITEBLOCKSIZE=${SNODEC_DEFAULT_WRITEBLOCKSIZE};\
SNODEC_DEFAULT_READTIMEOUT=${SNODEC_DEFAULT_READTIMEOUT};\
SNODEC_DEFAULT_WRITETIMEOUT=${SNODEC_DEFAULT_WRITETIMEOUT};\
SNODEC_DEFAULT_TERMINATETIMEOUT=${SNODEC_DEFAULT_TERMINATETIMEOUT}"
)

set(SNODEC_DEFAULT_TLS_INITTIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_TLS_INITTIMEOUT},>,10,${SNODEC_DEFAULT_TLS_INITTIMEOUT}>
)
set(SNODEC_DEFAULT_TLS_SHUTDOWNTIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_TLS_SHUTDOWNTIMEOUT},>,2,${SNODEC_DEFAULT_TLS_SHUTDOWNTIMEOUT}>
)
set_source_files_properties(
    config/ConfigTls.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "SNODEC_DEFAULT_TLS_INITTIMEOUT=${SNODEC_DEFAULT_TLS_INITTIMEOUT};\
SNODEC_DEFAULT_TLS_SHUTDOWNTIMEOUT=${SNODEC_DEFAULT_TLS_SHUTDOWNTIMEOUT}"
)

set(SNODEC_DEFAULT_ACCEPTS_PER_TICK
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_ACCEPTS_PER_TICK},>,1,${SNODEC_DEFAULT_ACCEPTS_PER_TICK}>
)
set(SNODEC_DEFAULT_BACKLOG
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_BACKLOG},>,5,${SNODEC_DEFAULT_BACKLOG}>
)
set_source_files_properties(
    config/ConfigListen.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "SNODEC_DEFAULT_ACCEPTS_PER_TICK=${SNODEC_DEFAULT_ACCEPTS_PER_TICK};\
SNODEC_DEFAULT_BACKLOG=${SNODEC_DEFAULT_BACKLOG}"
)

set(SNODEC_DEFAULT_REUSE_ADDRESS
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_REUSE_ADDRESS},>,"false","true">
)
set(SNODEC_DEFAULT_RETRY
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_RETRY},>,"false","true">
)
set(SNODEC_DEFAULT_RETRY_TIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_RETRY_TIMEOUT},>,1,${SNODEC_DEFAULT_RETRY_TIMEOUT}>
)
set(SNODEC_DEFAULT_RETRY_TRIES
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_RETRY_TRIES},>,0,${SNODEC_DEFAULT_RETRY_TRIES}>
)
set(SNODEC_DEFAULT_RETRY_BASE
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_RETRY_BASE},>,1.8,${SNODEC_DEFAULT_RETRY_BASE}>
)
set(SNODEC_DEFAULT_RETRY_JITTER
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_RETRY_JITTER},>,0,${SNODEC_DEFAULT_RETRY_JITTER}>
)
set(SNODEC_DEFAULT_TLS_INITTIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_TLS_INITTIMEOUT},>,10,${SNODEC_DEFAULT_TLS_INITTIMEOUT}>
)
set(SNODEC_DEFAULT_RETRY_LIMIT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_RETRY_LIMIT},>,0,${SNODEC_DEFAULT_RETRY_LIMIT}>
)
set_source_files_properties(
    config/ConfigPhysicalSocket.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "SNODEC_DEFAULT_REUSE_ADDRESS=${SNODEC_DEFAULT_REUSE_ADDRESS};\
SNODEC_DEFAULT_RETRY=${SNODEC_DEFAULT_RETRY};\
SNODEC_DEFAULT_RETRY_TIMEOUT=${SNODEC_DEFAULT_RETRY_TIMEOUT};\
SNODEC_DEFAULT_RETRY_TRIES=${SNODEC_DEFAULT_RETRY_TRIES};\
SNODEC_DEFAULT_RETRY_BASE=${SNODEC_DEFAULT_RETRY_BASE};\
SNODEC_DEFAULT_RETRY_JITTER=${SNODEC_DEFAULT_RETRY_JITTER};\
SNODEC_DEFAULT_TLS_INITTIMEOUT=${SNODEC_DEFAULT_TLS_INITTIMEOUT};\
SNODEC_DEFAULT_RETRY_LIMIT=${SNODEC_DEFAULT_RETRY_LIMIT}"
)

set(SNODEC_DEFAULT_ACCEPT_TIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_ACCEPT_TIMEOUT},>,0,${SNODEC_DEFAULT_ACCEPT_TIMEOUT}>
)
set_source_files_properties(
    config/ConfigPhysicalSocketServer.cpp
    PROPERTIES COMPILE_DEFINITIONS
               SNODEC_DEFAULT_ACCEPT_TIMEOUT=${SNODEC_DEFAULT_ACCEPT_TIMEOUT}
)

set(SNODEC_DEFAULT_CONNECT_TIMEOUT
    $<IF:$<STREQUAL:${SNODEC_DEFAULT_CONNECT_TIMEOUT},>,10,${SNODEC_DEFAULT_CONNECT_TIMEOUT}>
)
set_source_files_properties(
    config/ConfigPhysicalSocketClient.cpp
    PROPERTIES COMPILE_DEFINITIONS
               SNODEC_DEFAULT_CONNECT_TIMEOUT=${SNODEC_DEFAULT_CONNECT_TIMEOUT}
)

add_library(net SHARED ${NET_CPP} ${NET_H})
add_library(snodec::net ALIAS net)

target_include_directories(
    net
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net PUBLIC snodec::core)

set_target_properties(
    net PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME snodec-net
)

install(
    TARGETS net
    EXPORT snodec_net_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net
    COMPONENT net
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "in" EXCLUDE
    PATTERN "in6" EXCLUDE
    PATTERN "l2" EXCLUDE
    PATTERN "rc" EXCLUDE
    PATTERN "un" EXCLUDE
)

install(
    EXPORT snodec_net_Targets
    FILE snodec_net_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net
)

add_subdirectory(in)
add_subdirectory(in6)
add_subdirectory(l2)
add_subdirectory(rc)
add_subdirectory(un)
