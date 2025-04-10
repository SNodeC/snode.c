# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# ---------------------------------------------------------------------------
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.18)

set(IO_MULTIPLEXERS "epoll;poll;select")
if("${IO_MULTIPLEXER}" STREQUAL "")
    list(SUBLIST IO_MULTIPLEXERS 0 1 IO_MULTIPLEXER)
endif("${IO_MULTIPLEXER}" STREQUAL "")
set(SNODEC_IO_MULTIPLEXER
    ${IO_MULTIPLEXER}
    CACHE STRING "I/O-Multiplexer to use"
)
set_property(CACHE SNODEC_IO_MULTIPLEXER PROPERTY STRINGS ${IO_MULTIPLEXERS})
message(STATUS "I/O-Multiplexer selected: '${SNODEC_IO_MULTIPLEXER}'")

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
    SNodeC.cpp
    State.cpp
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
    SNodeC.h
    State.h
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
    core PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    core
    PRIVATE mux-${SNODEC_IO_MULTIPLEXER}
    PUBLIC utils ${CMAKE_DL_LIBS}
)

set_target_properties(
    core
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-core
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
