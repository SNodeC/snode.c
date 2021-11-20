cmake_minimum_required(VERSION 3.0)

add_library(
    core SHARED
    AcceptEventReceiver.cpp
    ConnectEventReceiver.cpp
    Descriptor.cpp
    DynamicLoader.cpp
    EventDispatcher.cpp
    EventLoop.cpp
    EventReceiver.cpp
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
    system/dlfcn.cpp
    system/netdb.cpp
    system/select.cpp
    system/signal.cpp
    system/socket.cpp
    system/time.cpp
    system/unistd.cpp
    timer/Timer.cpp
)
add_library(snodec::core ALIAS core)

target_include_directories(
    core
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(core PUBLIC snodec::logger snodec::utils)

set_target_properties(core PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-core)

install(
    TARGETS core
    EXPORT snodeccoreTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "socket/ip" EXCLUDE
    PATTERN "socket/bluetooth" EXCLUDE
    PATTERN "socket/stream/tls" EXCLUDE
    PATTERN "socket/stream/legacy" EXCLUDE
)

install(
    EXPORT snodeccoreTargets
    FILE snodeccoreTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(socket/stream/legacy)
add_subdirectory(socket/stream/tls)
