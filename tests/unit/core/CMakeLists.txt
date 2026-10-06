# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

snodec_add_test(SNodeCInitFreeSmokeTest SNodeCInitFreeSmokeTest.cpp)

target_link_libraries(
    SNodeCInitFreeSmokeTest PRIVATE snodec-test-support snodec::core
)

set_tests_properties(
    SNodeCInitFreeSmokeTest PROPERTIES LABELS "unit;core;smoke"
)

snodec_add_test(SNodeCAsyncLogDrainTest SNodeCAsyncLogDrainTest.cpp)

target_link_libraries(
    SNodeCAsyncLogDrainTest PRIVATE snodec-test-support snodec::core
)

set_tests_properties(
    SNodeCAsyncLogDrainTest PROPERTIES LABELS "unit;core;log;lifecycle"
)

snodec_add_test(TLSResultClassificationTest TLSResultClassificationTest.cpp)

target_include_directories(
    TLSResultClassificationTest PRIVATE ${PROJECT_SOURCE_DIR}/src
                                        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    TLSResultClassificationTest PRIVATE snodec-test-support
                                        snodec::core-socket-stream-tls
)

target_compile_features(TLSResultClassificationTest PRIVATE cxx_std_20)

set_tests_properties(
    TLSResultClassificationTest
    PROPERTIES LABELS "unit;core;tls;classification" TIMEOUT 5
)

snodec_add_test(
    TLSLifecycleAbiSourceCompatibilityTest
    TLSLifecycleAbiSourceCompatibilityTest.cpp
)

target_link_libraries(
    TLSLifecycleAbiSourceCompatibilityTest
    PRIVATE snodec::core-socket-stream-tls
)

target_compile_features(
    TLSLifecycleAbiSourceCompatibilityTest PRIVATE cxx_std_20
)

set_tests_properties(
    TLSLifecycleAbiSourceCompatibilityTest
    PROPERTIES LABELS "unit;core;tls;abi" TIMEOUT 5
)

snodec_add_test(ListenerLifecycleTest ListenerLifecycleTest.cpp)
target_include_directories(ListenerLifecycleTest PRIVATE ${PROJECT_SOURCE_DIR})
target_compile_features(ListenerLifecycleTest PRIVATE cxx_std_20)
target_compile_definitions(ListenerLifecycleTest PRIVATE SNODEC_INTREE_BUILD)
target_link_libraries(
    ListenerLifecycleTest PRIVATE snodec-test-support snodec::net
                                  snodec::core-socket-stream
)
set_tests_properties(
    ListenerLifecycleTest PROPERTIES LABELS "unit;core;lifecycle;listener"
)

add_executable(EndpointLifetimeCountersTest EndpointLifetimeCountersTest.cpp)
target_include_directories(
    EndpointLifetimeCountersTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_compile_features(EndpointLifetimeCountersTest PRIVATE cxx_std_20)
target_link_libraries(
    EndpointLifetimeCountersTest PRIVATE snodec-test-support snodec::net
                                         snodec::core-socket-stream
)
foreach(
    scenario IN
    ITEMS server-terminal
          server-retry
          server-retry-cancelled
          client-terminal
          client-retry
          client-retry-cancelled
          client-disconnect-terminal
          client-reconnect-accepted
          client-reconnect-cancelled
          server-sequence
          client-sequence
          expired-weak-context
          independent-server-retries
          independent-client-retries
          independent-client-reconnects
)
    add_test(NAME EndpointLifetimeCountersTest-${scenario}
             COMMAND EndpointLifetimeCountersTest ${scenario}
    )
    set_tests_properties(
        EndpointLifetimeCountersTest-${scenario}
        PROPERTIES LABELS "unit;core;lifecycle;endpoint-counter" TIMEOUT 10
    )
endforeach()

snodec_add_test(TlsOpenSslLoggingTest TlsOpenSslLoggingTest.cpp)
target_include_directories(TlsOpenSslLoggingTest PRIVATE ${PROJECT_SOURCE_DIR})
target_compile_features(TlsOpenSslLoggingTest PRIVATE cxx_std_20)
target_link_libraries(
    TlsOpenSslLoggingTest PRIVATE snodec-test-support
                                  snodec::core-socket-stream-tls
)
set_tests_properties(
    TlsOpenSslLoggingTest PROPERTIES LABELS "unit;core;tls;log"
)

add_executable(StreamFrameworkShutdownTest StreamFrameworkShutdownTest.cpp)

target_include_directories(
    StreamFrameworkShutdownTest PRIVATE ${PROJECT_SOURCE_DIR}/src
                                        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(
    StreamFrameworkShutdownTest PRIVATE snodec-test-support
                                        snodec::core-socket-stream snodec::net
)

target_compile_features(StreamFrameworkShutdownTest PRIVATE cxx_std_20)

foreach(
    scenario
    plain_requested_empty
    plain_requested_queued
    plain_signal_false_veto
    plain_no_observer
    plain_noncooperating_timeout
    reentrant_context_cleanup
    active_detach_context_visibility
    destruction_logging_order
)
    add_test(NAME StreamFrameworkShutdown_${scenario}
             COMMAND StreamFrameworkShutdownTest ${scenario}
    )

    set_tests_properties(
        StreamFrameworkShutdown_${scenario}
        PROPERTIES LABELS "unit;core;stream;lifecycle;shutdown" TIMEOUT 8
    )
endforeach()

snodec_add_test(
    SocketWriterResourcePolicyTest SocketWriterResourcePolicyTest.cpp
)
target_include_directories(
    SocketWriterResourcePolicyTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_compile_features(SocketWriterResourcePolicyTest PRIVATE cxx_std_20)
target_link_libraries(
    SocketWriterResourcePolicyTest
    PRIVATE snodec-test-support snodec::core-socket-stream snodec::core
            snodec::net
)
set_tests_properties(
    SocketWriterResourcePolicyTest
    PROPERTIES LABELS "unit;core;stream;writer;resource-policy" TIMEOUT 5
)

foreach(test IN ITEMS TLSIoFatalPathTest SocketContextLifecycleTest)
    add_executable(${test} ${test}.cpp)
    target_include_directories(${test} PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(
        ${test} PRIVATE snodec-test-support snodec::net-in-stream-tls
                        snodec::net-in-stream-legacy
    )
endforeach()
add_test(NAME TLSIoFatalPathTest-eof COMMAND TLSIoFatalPathTest eof)
add_test(NAME TLSIoFatalPathTest-reset COMMAND TLSIoFatalPathTest reset)
add_test(NAME SocketContextLifecycleTest COMMAND SocketContextLifecycleTest)
set_tests_properties(
    TLSIoFatalPathTest-eof TLSIoFatalPathTest-reset SocketContextLifecycleTest
    PROPERTIES TIMEOUT 20
)

foreach(test IN ITEMS TLSLifecycleHelperOwnershipTest
                      TLSTransportStateMachineTest TLSFrameworkShutdownTest
)
    add_executable(${test} ${test}.cpp)
    target_include_directories(${test} PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(
        ${test} PRIVATE snodec-test-support snodec::net-in-stream-tls
    )
endforeach()
foreach(scenario IN ITEMS success timeout invalid shutdown)
    add_test(NAME TLSLifecycleHelperOwnershipTest-${scenario}
             COMMAND TLSLifecycleHelperOwnershipTest ${scenario}
    )
    set_tests_properties(
        TLSLifecycleHelperOwnershipTest-${scenario} PROPERTIES TIMEOUT 20
    )
endforeach()
foreach(scenario IN ITEMS close plaintext)
    add_test(NAME TLSTransportStateMachineTest-${scenario}
             COMMAND TLSTransportStateMachineTest ${scenario}
    )
    set_tests_properties(
        TLSTransportStateMachineTest-${scenario} PROPERTIES TIMEOUT 20
    )
endforeach()
foreach(scenario IN ITEMS cooperate stall)
    add_test(NAME TLSFrameworkShutdownTest-${scenario}
             COMMAND TLSFrameworkShutdownTest ${scenario}
    )
    set_tests_properties(
        TLSFrameworkShutdownTest-${scenario} PROPERTIES TIMEOUT 20
    )
endforeach()

add_executable(
    ConnectionAttemptLifecycleTest ConnectionAttemptLifecycleTest.cpp
)
target_include_directories(
    ConnectionAttemptLifecycleTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_link_libraries(
    ConnectionAttemptLifecycleTest PRIVATE OpenSSL::SSL snodec-test-support
                                           snodec::net-in-stream-legacy
)
foreach(scenario IN ITEMS retry cancel)
    add_test(NAME ConnectionAttemptLifecycleTest-${scenario}
             COMMAND ConnectionAttemptLifecycleTest ${scenario}
    )
    set_tests_properties(
        ConnectionAttemptLifecycleTest-${scenario} PROPERTIES TIMEOUT 20
    )
endforeach()
