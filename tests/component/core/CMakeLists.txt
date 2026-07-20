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

snodec_add_test(SingleshotTimerTest SingleshotTimerTest.cpp)
snodec_add_test(IntervalTimerStopableTest IntervalTimerStopableTest.cpp)
snodec_add_test(SNodeCStopFromCallbackTest SNodeCStopFromCallbackTest.cpp)
add_executable(
    ShutdownReceiverNotificationTest ShutdownReceiverNotificationTest.cpp
)
snodec_add_test(TimerCancelBeforeFireTest TimerCancelBeforeFireTest.cpp)
snodec_add_test(
    IntervalTimerCancelFromCallbackTest IntervalTimerCancelFromCallbackTest.cpp
)
snodec_add_test(PipeBoundedQueueTest PipeBoundedQueueTest.cpp)
snodec_add_test(PipeOwnershipTest PipeOwnershipTest.cpp)
snodec_add_test(PipeImmediateCloseTest PipeImmediateCloseTest.cpp)
snodec_add_test(PipeSinkFairnessTest PipeSinkFairnessTest.cpp)
snodec_add_test(PipeTimeoutTest PipeTimeoutTest.cpp)
snodec_add_test(
    DescriptorRegistrationFailureTest DescriptorRegistrationFailureTest.cpp
)

target_link_libraries(
    SingleshotTimerTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(
    IntervalTimerStopableTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(
    SNodeCStopFromCallbackTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(
    ShutdownReceiverNotificationTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(
    TimerCancelBeforeFireTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(
    IntervalTimerCancelFromCallbackTest PRIVATE snodec-test-support
                                                snodec::core
)
target_link_libraries(
    PipeBoundedQueueTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(
    PipeOwnershipTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(
    PipeImmediateCloseTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(
    PipeSinkFairnessTest PRIVATE snodec-test-support snodec::core
)
target_link_libraries(PipeTimeoutTest PRIVATE snodec-test-support snodec::core)
target_link_libraries(
    DescriptorRegistrationFailureTest PRIVATE snodec-test-support snodec::core
)

target_compile_features(SingleshotTimerTest PRIVATE cxx_std_20)
target_compile_features(IntervalTimerStopableTest PRIVATE cxx_std_20)
target_compile_features(SNodeCStopFromCallbackTest PRIVATE cxx_std_20)
target_compile_features(ShutdownReceiverNotificationTest PRIVATE cxx_std_20)
target_compile_features(TimerCancelBeforeFireTest PRIVATE cxx_std_20)
target_compile_features(IntervalTimerCancelFromCallbackTest PRIVATE cxx_std_20)
target_compile_features(PipeBoundedQueueTest PRIVATE cxx_std_20)
target_compile_features(PipeOwnershipTest PRIVATE cxx_std_20)
target_compile_features(PipeImmediateCloseTest PRIVATE cxx_std_20)
target_compile_features(PipeSinkFairnessTest PRIVATE cxx_std_20)
target_compile_features(PipeTimeoutTest PRIVATE cxx_std_20)
target_compile_features(DescriptorRegistrationFailureTest PRIVATE cxx_std_20)

set_tests_properties(
    SingleshotTimerTest PROPERTIES LABELS "component;core;timer"
                                   SKIP_RETURN_CODE 77
)
set_tests_properties(
    IntervalTimerStopableTest PROPERTIES LABELS "component;core;timer"
                                         SKIP_RETURN_CODE 77
)
set_tests_properties(
    SNodeCStopFromCallbackTest PROPERTIES LABELS "component;core;lifecycle"
                                          SKIP_RETURN_CODE 77 TIMEOUT 5
)
foreach(scenario IN ITEMS requested signal)
    add_test(NAME ShutdownReceiverNotification_${scenario}
             COMMAND ShutdownReceiverNotificationTest ${scenario}
    )
    set_tests_properties(
        ShutdownReceiverNotification_${scenario}
        PROPERTIES LABELS "component;core;lifecycle;shutdown" SKIP_RETURN_CODE
                   77 TIMEOUT 5
    )
endforeach()
set_tests_properties(
    TimerCancelBeforeFireTest PROPERTIES LABELS "component;core;timer"
                                         SKIP_RETURN_CODE 77 TIMEOUT 5
)
set_tests_properties(
    IntervalTimerCancelFromCallbackTest
    PROPERTIES LABELS "component;core;timer" SKIP_RETURN_CODE 77 TIMEOUT 5
)
set_tests_properties(
    PipeBoundedQueueTest PROPERTIES LABELS "component;core;pipe"
                                    SKIP_RETURN_CODE 77 TIMEOUT 5
)
set_tests_properties(
    PipeOwnershipTest PROPERTIES LABELS "component;core;pipe" SKIP_RETURN_CODE
                                 77 TIMEOUT 5
)
set_tests_properties(
    PipeImmediateCloseTest PROPERTIES LABELS "component;core;pipe"
                                      SKIP_RETURN_CODE 77 TIMEOUT 5
)
set_tests_properties(
    PipeSinkFairnessTest PROPERTIES LABELS "component;core;pipe"
                                    SKIP_RETURN_CODE 77 TIMEOUT 5
)
set_tests_properties(
    PipeTimeoutTest PROPERTIES LABELS "component;core;pipe" SKIP_RETURN_CODE 77
                               TIMEOUT 5
)
set_tests_properties(
    DescriptorRegistrationFailureTest
    PROPERTIES LABELS "component;core;descriptor" SKIP_RETURN_CODE 77 TIMEOUT 5
)
