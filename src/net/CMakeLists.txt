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

append_source_file_config(
    config/ConfigConnection.cpp SNODEC_DEFAULT_READBLOCKSIZE
    "Read block size in bytes" 16384
)
append_source_file_config(
    config/ConfigConnection.cpp SNODEC_DEFAULT_WRITEBLOCKSIZE
    "Write block size in bytes" 16384
)
append_source_file_config(
    config/ConfigConnection.cpp SNODEC_DEFAULT_READTIMEOUT
    "Read inactivity timeout in seconds" 60
)
append_source_file_config(
    config/ConfigConnection.cpp SNODEC_DEFAULT_WRITETIMEOUT
    "Write inactivity timeout in seconds" 60
)
append_source_file_config(
    config/ConfigConnection.cpp SNODEC_DEFAULT_TERMINATETIMEOUT
    "Shutdown inactivity timeout in seconds" 1
)

append_source_file_config(
    config/ConfigListen.cpp SNODEC_DEFAULT_ACCEPTS_PER_TICK "Accepts per tick"
    1
)
append_source_file_config(
    config/ConfigListen.cpp SNODEC_DEFAULT_BACKLOG "Listen backlog" 5
)

append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_REUSE_ADDRESS
    "Reuse address" \"false\"
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_RECONNECT
    "Reconnect after disconnect" \"false\"
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_RECONNECT_TIME
    "Reconnect time in seconds" 1
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_RETRY
    "Retry listen and connect" \"false\"
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_RETRY_TIMEOUT
    "Retry interval in seconds" 1
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_RETRY_TRIES
    "Upper limit of retry tries" 0
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_RETRY_BASE
    "Base of exponential backoff" 1.8
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_RETRY_JITTER
    "Jitter of retry timeout in percent" 0
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp SNODEC_DEFAULT_RETRY_LIMIT
    "Upper limit of retry timeout" 0
)

append_source_file_config(
    config/ConfigPhysicalSocketServer.cpp SNODEC_DEFAULT_ACCEPT_TIMEOUT
    "Accept inactivity timeout in seconds" 0
)
append_source_file_config(
    config/ConfigPhysicalSocketClient.cpp SNODEC_DEFAULT_CONNECT_TIMEOUT
    "Connect timeout in seconds" 10
)

append_source_file_config(
    config/ConfigTls.cpp SNODEC_DEFAULT_TLS_INITTIMEOUT
    "SSL/TLS initial handshake timeout in seconds" 10
)
append_source_file_config(
    config/ConfigTls.cpp SNODEC_DEFAULT_TLS_SHUTDOWNTIMEOUT
    "SSL/TLS teardown timeout in seconds" 1
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
