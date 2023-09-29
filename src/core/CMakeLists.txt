cmake_minimum_required(VERSION 3.18)

add_subdirectory(multiplexer)

set(CORE_CPP
    Descriptor.cpp
    DescriptorEventPublisher.cpp
    DescriptorEventReceiver.cpp
    DynamicLoader.cpp
    Event.cpp
    EventLoop.cpp
    EventMultiplexer.cpp
    EventReceiver.cpp
    ProgressLog.cpp
    SNodeC.cpp
    State.h
    Timer.cpp
    TimerEventPublisher.cpp
    TimerEventReceiver.cpp
    eventreceiver/AcceptEventReceiver.cpp
    eventreceiver/ConnectEventReceiver.cpp
    eventreceiver/ExceptionalConditionEventReceiver.cpp
    eventreceiver/ReadEventReceiver.cpp
    eventreceiver/WriteEventReceiver.cpp
    file/File.cpp
    file/FileReader.cpp
    pipe/Sink.cpp
    pipe/Source.cpp
    pipe/Pipe.cpp
    pipe/PipeSink.cpp
    pipe/PipeSource.cpp
    system/dlfcn.cpp
    system/epoll.cpp
    system/netdb.cpp
    system/poll.cpp
    system/select.cpp
    system/socket.cpp
    system/unistd.cpp
    timer/IntervalTimer.cpp
    timer/IntervalTimerStopable.cpp
    timer/SingleshotTimer.cpp
    timer/Timer.cpp
)

set(CORE_H
    Descriptor.h
    DescriptorEventPublisher.h
    DescriptorEventReceiver.h
    DynamicLoader.h
    Event.h
    EventLoop.h
    EventMultiplexer.h
    EventReceiver.h
    ProgressLog.h
    SNodeC.h
    TickStatus.h
    Timer.h
    TimerEventPublisher.h
    TimerEventReceiver.h
    eventreceiver/AcceptEventReceiver.h
    eventreceiver/ConnectEventReceiver.h
    eventreceiver/ExceptionalConditionEventReceiver.h
    eventreceiver/ReadEventReceiver.h
    eventreceiver/WriteEventReceiver.h
    file/File.h
    file/FileReader.h
    pipe/Sink.h
    pipe/Source.h
    pipe/Pipe.h
    pipe/PipeSink.h
    pipe/PipeSource.h
    system/dlfcn.h
    system/epoll.h
    system/netdb.h
    system/poll.h
    system/select.h
    system/socket.h
    system/unistd.h
    timer/IntervalTimer.h
    timer/IntervalTimerStopable.h
    timer/SingleshotTimer.h
    timer/Timer.h
)

set_source_files_properties(
    EventLoop.cpp
    PROPERTIES COMPILE_FLAGS
               $<$<CXX_COMPILER_ID:Clang>:-Wno-disabled-macro-expansion>
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
    core
    PRIVATE snodec::mux-${SNODEC_IO_MULTIPLEXER}
    PUBLIC snodec::logger snodec::utils ${CMAKE_DL_LIBS}
)

set_target_properties(
    core PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME snodec-core
)

install(
    TARGETS core
    EXPORT snodec_core_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core
    COMPONENT core
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "." EXCLUDE
    PATTERN "EventLoop.h" EXCLUDE
    PATTERN "EventMultiplexer.h" EXCLUDE
    PATTERN "DescriptorEventPublisher.h" EXCLUDE
    PATTERN "TimerEventPublisher.h" EXCLUDE
    PATTERN "timer/IntervalTimer.h" EXCLUDE
    PATTERN "timer/IntervalTimerStopable.h" EXCLUDE
    PATTERN "timer/SingleshoTimer.h" EXCLUDE
    PATTERN "cmake" EXCLUDE
    PATTERN "multiplexer" EXCLUDE
    PATTERN "socket" EXCLUDE
)

install(
    EXPORT snodec_core_Targets
    FILE snodec_core_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT core
)

add_subdirectory(socket)
