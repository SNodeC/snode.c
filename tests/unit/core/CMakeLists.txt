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

target_link_libraries(SNodeCInitFreeSmokeTest PRIVATE snodec-test-support snodec::core)

set_tests_properties(SNodeCInitFreeSmokeTest PROPERTIES LABELS "unit;core;smoke")



snodec_add_test(TLSResultClassificationTest TLSResultClassificationTest.cpp)
target_include_directories(TLSResultClassificationTest PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})
target_link_libraries(TLSResultClassificationTest PRIVATE snodec-test-support snodec::core-socket-stream-tls)
target_compile_features(TLSResultClassificationTest PRIVATE cxx_std_20)
set_tests_properties(TLSResultClassificationTest PROPERTIES LABELS "unit;core;tls;classification" TIMEOUT 5)

snodec_add_test(TLSIoFatalPathTest TLSIoFatalPathTest.cpp)
target_include_directories(TLSIoFatalPathTest PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})
target_link_libraries(TLSIoFatalPathTest PRIVATE snodec-test-support snodec::core-socket-stream-tls snodec::net)
target_compile_features(TLSIoFatalPathTest PRIVATE cxx_std_20)
target_compile_definitions(TLSIoFatalPathTest PRIVATE SNODEC_BUILD_TESTS)
set_tests_properties(TLSIoFatalPathTest PROPERTIES LABELS "unit;core;tls;io;fatal" TIMEOUT 5)

snodec_add_test(TLSLifecycleHelperOwnershipTest TLSLifecycleHelperOwnershipTest.cpp)

target_include_directories(TLSLifecycleHelperOwnershipTest PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})
target_link_libraries(TLSLifecycleHelperOwnershipTest PRIVATE snodec-test-support snodec::core-socket-stream-tls snodec::net)
target_compile_features(TLSLifecycleHelperOwnershipTest PRIVATE cxx_std_20)
target_compile_definitions(TLSLifecycleHelperOwnershipTest PRIVATE SNODEC_BUILD_TESTS)
set_tests_properties(TLSLifecycleHelperOwnershipTest PROPERTIES LABELS "unit;core;tls;lifecycle;ownership" TIMEOUT 5)

snodec_add_test(TLSTransportStateMachineTest TLSTransportStateMachineTest.cpp)
target_include_directories(TLSTransportStateMachineTest PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})
target_link_libraries(TLSTransportStateMachineTest PRIVATE snodec-test-support snodec::core-socket-stream-tls snodec::net)
target_compile_features(TLSTransportStateMachineTest PRIVATE cxx_std_20)
target_compile_definitions(TLSTransportStateMachineTest PRIVATE SNODEC_BUILD_TESTS)
set_tests_properties(TLSTransportStateMachineTest PROPERTIES LABELS "unit;core;tls;state-machine" TIMEOUT 5)


snodec_add_test(TLSLifecycleAbiSourceCompatibilityTest TLSLifecycleAbiSourceCompatibilityTest.cpp)
target_link_libraries(TLSLifecycleAbiSourceCompatibilityTest PRIVATE snodec::core-socket-stream-tls)
target_compile_features(TLSLifecycleAbiSourceCompatibilityTest PRIVATE cxx_std_20)
set_tests_properties(TLSLifecycleAbiSourceCompatibilityTest PROPERTIES LABELS "unit;core;tls;abi" TIMEOUT 5)

add_executable(StreamFrameworkShutdownTest StreamFrameworkShutdownTest.cpp)
target_include_directories(StreamFrameworkShutdownTest PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})
target_link_libraries(StreamFrameworkShutdownTest PRIVATE snodec-test-support snodec::core-socket-stream snodec::net)
target_compile_features(StreamFrameworkShutdownTest PRIVATE cxx_std_20)
target_compile_definitions(StreamFrameworkShutdownTest PRIVATE SNODEC_BUILD_TESTS)

foreach(scenario
        plain_requested_empty
        plain_requested_queued
        plain_signal_false_veto
        plain_no_observer
        plain_noncooperating_timeout
        reentrant_context_cleanup
        reentrant_context_finalizer_cleanup
        destruction_logging_order)
    add_test(NAME StreamFrameworkShutdown_${scenario} COMMAND StreamFrameworkShutdownTest ${scenario})
    set_tests_properties(
        StreamFrameworkShutdown_${scenario}
        PROPERTIES LABELS "unit;core;stream;lifecycle;shutdown" TIMEOUT 8)
endforeach()

add_executable(TLSFrameworkShutdownTest TLSFrameworkShutdownTest.cpp)
target_include_directories(TLSFrameworkShutdownTest PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR})
target_link_libraries(
    TLSFrameworkShutdownTest
    PRIVATE snodec-test-support snodec::core-socket-stream-tls snodec::net)
target_compile_features(TLSFrameworkShutdownTest PRIVATE cxx_std_20)
target_compile_definitions(TLSFrameworkShutdownTest PRIVATE SNODEC_BUILD_TESTS)

foreach(scenario
        active_helper_requested
        active_helper_no_observer
        active_helper_signal
        active_helper_forced
        publisher_mutation_same_list
        publisher_mutation_cross_publisher
        signal_false_tls
        outer_timeout_long
        outer_timeout_multiple)
    add_test(NAME TLSFrameworkShutdown_${scenario} COMMAND TLSFrameworkShutdownTest ${scenario})
    set_tests_properties(
        TLSFrameworkShutdown_${scenario}
        PROPERTIES LABELS "unit;core;stream;tls;lifecycle;shutdown" TIMEOUT 8)
endforeach()
