cmake_minimum_required(VERSION 3.0)

find_package(range-v3 REQUIRED)

add_library(
    net SHARED
    AcceptEventReceiver.cpp
    ConnectEventReceiver.cpp
    Descriptor.cpp
    DescriptorEventDispatcher.cpp
    DescriptorEventReceiver.cpp
    EventLoop.cpp
    FdSet.cpp
    ExceptionalConditionEventReceiver.cpp
    ReadEventReceiver.cpp
    SNodeC.cpp
    TimerEventDispatcher.cpp
    WriteEventReceiver.cpp
    file/File.cpp
    file/FileReader.cpp
    pipe/Source.cpp
    pipe/Sink.cpp
    pipe/Pipe.cpp
    pipe/PipeSink.cpp
    pipe/PipeSource.cpp
    socket/stream/SocketConnection.cpp
    socket/stream/SocketContext.cpp
    system/netdb.cpp
    system/select.cpp
    system/signal.cpp
    system/socket.cpp
    system/time.cpp
    system/unistd.cpp
    timer/Timer.cpp
)
add_library(snodec::net ALIAS net)

target_link_libraries(net PUBLIC snodec::logger snodec::utils)

set_target_properties(
    net
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_include_directories(
    net
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

install(
    TARGETS net
    EXPORT snodecnetTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "socket/ip" EXCLUDE
    PATTERN "socket/bluetooth" EXCLUDE
    PATTERN "socket/stream/tls" EXCLUDE
)

install(
    EXPORT snodecnetTargets
    FILE snodecnetTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(socket/stream/tls)
add_subdirectory(socket/ip)
add_subdirectory(socket/bluetooth)
