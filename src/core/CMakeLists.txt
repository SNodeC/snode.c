cmake_minimum_required(VERSION 3.0)

set(CORE_CPP
    AcceptEventReceiver.cpp
    ConnectEventReceiver.cpp
    Descriptor.cpp
    DescriptorEventPublisher.cpp
    DescriptorEventReceiver.cpp
    DynamicLoader.cpp
    Event.cpp
    EventLoop.cpp
    EventMultiplexer.cpp
    EventReceiver.cpp
    ExceptionalConditionEventReceiver.cpp
    InitAcceptEventReceiver.cpp
    InitConnectEventReceiver.cpp
    ReadEventReceiver.cpp
    SNodeC.cpp
    TimerEventDispatcher.cpp
    WriteEventReceiver.cpp
    file/File.cpp
    file/FileReader.cpp
    pipe/Sink.cpp
    pipe/Source.cpp
    pipe/Pipe.cpp
    pipe/PipeSink.cpp
    pipe/PipeSource.cpp
    socket/SocketConnection.cpp
    socket/SocketContext.cpp
    system/dlfcn.cpp
    system/epoll.cpp
    system/netdb.cpp
    system/poll.cpp
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
    DescriptorEventPublisher.h
    DescriptorEventReceiver.h
    DynamicLoader.h
    Event.h
    EventLoop.h
    EventMultiplexer.h
    EventReceiver.h
    ExceptionalConditionEventReceiver.h
    InitAcceptEventReceiver.h
    InitConnectEventReceiver.h
    ReadEventReceiver.h
    SNodeC.h
    TickStatus.h
    TimerEventDispatcher.h
    TimerEventReceiver.h
    WriteEventReceiver.h
    file/File.h
    file/FileReader.h
    pipe/Sink.h
    pipe/Source.h
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
    system/epoll.h
    system/netdb.h
    system/poll.h
    system/select.h
    system/signal.h
    system/socket.h
    system/time.h
    system/unistd.h
    timer/IntervalTimer.h
    timer/SingleshotTimer.h
    timer/Timer.h
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
    core PUBLIC snodec::mux-${IO_Multiplexer} snodec::logger snodec::utils
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
    PATTERN "mux" EXCLUDE
)

install(
    EXPORT snodec_core_Targets
    FILE snodec_core_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(mux)

add_subdirectory(socket/stream/legacy)
add_subdirectory(socket/stream/tls)
