cmake_minimum_required(VERSION 3.0)

set(CORE_CPP
    AcceptEventReceiver.cpp
    ConnectEventReceiver.cpp
    Descriptor.cpp
    DescriptorEventDispatcher.cpp
    DynamicLoader.cpp
    EventLoop.cpp
    EventReceiver.cpp
    EventReceiver.h
    ExceptionalConditionEventReceiver.cpp
    ReadEventReceiver.cpp
    SNodeC.cpp
    WriteEventReceiver.cpp
    file/File.cpp
    file/FileReader.cpp
    pipe/Source.cpp
    pipe/Sink.cpp
    pipe/Pipe.cpp
    pipe/PipeSink.cpp
    pipe/PipeSource.cpp
    socket/SocketConnection.cpp
    socket/SocketContext.cpp
    system/dlfcn.cpp
    system/netdb.cpp
    system/select.cpp
    system/signal.cpp
    system/socket.cpp
    system/time.cpp
    system/unistd.cpp
    timer/Timer.cpp
)

set(CORE_H
    AcceptEventReceiver.h
    ConnectEventReceiver.h
    Descriptor.h
    DescriptorEventDispatcher.h
    DynamicLoader.h
    EventDispatcher.h
    EventLoop.h
    EventReceiver.h
    ExceptionalConditionEventReceiver.h
    ReadEventReceiver.h
    SNodeC.h
    TickStatus.h
    WriteEventReceiver.h
    file/File.h
    file/FileReader.h
    pipe/Source.h
    pipe/Sink.h
    pipe/Pipe.h
    pipe/PipeSink.h
    pipe/PipeSource.h
    socket/ClientSocket.h
    socket/ServerSocket.h
    socket/Socket.h
    socket/SocketAddress.h
    socket/SocketConnection.h
    socket/SocketContext.h
    socket/SocketContextFactory.h
    socket/stream/SocketAcceptor.h
    socket/stream/SocketClient.h
    socket/stream/SocketConnection.h
    socket/stream/SocketConnector.h
    socket/stream/SocketReader.h
    socket/stream/SocketServer.h
    socket/stream/SocketWriter.h
    system/dlfcn.h
    system/netdb.h
    system/select.h
    system/signal.h
    system/socket.h
    system/time.h
    system/unistd.h
    timer/Timer.h
    timer/IntervalTimer.h
    timer/SingleshotTimer.h
)

add_library(core SHARED ${CORE_CPP} ${CORE_H})
add_library(snodec::core ALIAS core)

target_include_directories(
    core
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    core PUBLIC snodec::core-${IO_Multiplexer} snodec::logger snodec::utils
)

set_target_properties(core PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-core)

install(
    TARGETS core
    EXPORT snodec_core_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "socket/stream/tls" EXCLUDE
    PATTERN "socket/stream/legacy" EXCLUDE
    PATTERN "select" EXCLUDE
    PATTERN "poll" EXCLUDE
    PATTERN "epoll" EXCLUDE
)

install(
    EXPORT snodec_core_Targets
    FILE snodec_core_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(epoll)
add_subdirectory(poll)
add_subdirectory(select)

add_subdirectory(socket/stream/legacy)
add_subdirectory(socket/stream/tls)
