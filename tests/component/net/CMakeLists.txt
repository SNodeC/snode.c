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


snodec_add_test(InetLegacyServerClientCompositionTest InetLegacyServerClientCompositionTest.cpp)

target_link_libraries(InetLegacyServerClientCompositionTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_compile_features(InetLegacyServerClientCompositionTest PRIVATE cxx_std_20)

set_tests_properties(InetLegacyServerClientCompositionTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;composition"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(Inet6LegacyServerClientCompositionTest Inet6LegacyServerClientCompositionTest.cpp)

target_link_libraries(Inet6LegacyServerClientCompositionTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_compile_features(Inet6LegacyServerClientCompositionTest PRIVATE cxx_std_20)

set_tests_properties(Inet6LegacyServerClientCompositionTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;composition"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(UnixLegacyServerClientCompositionTest UnixLegacyServerClientCompositionTest.cpp)

target_link_libraries(UnixLegacyServerClientCompositionTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_compile_features(UnixLegacyServerClientCompositionTest PRIVATE cxx_std_20)

set_tests_properties(UnixLegacyServerClientCompositionTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;composition"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(InetLegacyServerMultipleClientsCompositionTest InetLegacyServerMultipleClientsCompositionTest.cpp)
snodec_add_test(Inet6LegacyServerMultipleClientsCompositionTest Inet6LegacyServerMultipleClientsCompositionTest.cpp)
snodec_add_test(UnixLegacyServerMultipleClientsCompositionTest UnixLegacyServerMultipleClientsCompositionTest.cpp)

target_link_libraries(InetLegacyServerMultipleClientsCompositionTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(Inet6LegacyServerMultipleClientsCompositionTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(UnixLegacyServerMultipleClientsCompositionTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_compile_features(InetLegacyServerMultipleClientsCompositionTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyServerMultipleClientsCompositionTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyServerMultipleClientsCompositionTest PRIVATE cxx_std_20)

set_tests_properties(InetLegacyServerMultipleClientsCompositionTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;multi-client;composition"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6LegacyServerMultipleClientsCompositionTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;multi-client;composition"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixLegacyServerMultipleClientsCompositionTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;multi-client;composition"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(InetLegacyServerClientMultiMessagePayloadExchangeTest InetLegacyServerClientMultiMessagePayloadExchangeTest.cpp)
snodec_add_test(Inet6LegacyServerClientMultiMessagePayloadExchangeTest Inet6LegacyServerClientMultiMessagePayloadExchangeTest.cpp)
snodec_add_test(UnixLegacyServerClientMultiMessagePayloadExchangeTest UnixLegacyServerClientMultiMessagePayloadExchangeTest.cpp)

target_link_libraries(InetLegacyServerClientMultiMessagePayloadExchangeTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(Inet6LegacyServerClientMultiMessagePayloadExchangeTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(UnixLegacyServerClientMultiMessagePayloadExchangeTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_compile_features(InetLegacyServerClientMultiMessagePayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyServerClientMultiMessagePayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyServerClientMultiMessagePayloadExchangeTest PRIVATE cxx_std_20)

set_tests_properties(InetLegacyServerClientMultiMessagePayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;multi-message;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6LegacyServerClientMultiMessagePayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;multi-message;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixLegacyServerClientMultiMessagePayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;multi-message;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(InetLegacyServerMultipleClientsPayloadExchangeTest InetLegacyServerMultipleClientsPayloadExchangeTest.cpp)
snodec_add_test(Inet6LegacyServerMultipleClientsPayloadExchangeTest Inet6LegacyServerMultipleClientsPayloadExchangeTest.cpp)
snodec_add_test(UnixLegacyServerMultipleClientsPayloadExchangeTest UnixLegacyServerMultipleClientsPayloadExchangeTest.cpp)

target_link_libraries(InetLegacyServerMultipleClientsPayloadExchangeTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(Inet6LegacyServerMultipleClientsPayloadExchangeTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(UnixLegacyServerMultipleClientsPayloadExchangeTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_compile_features(InetLegacyServerMultipleClientsPayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyServerMultipleClientsPayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyServerMultipleClientsPayloadExchangeTest PRIVATE cxx_std_20)

set_tests_properties(InetLegacyServerMultipleClientsPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;multi-client;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6LegacyServerMultipleClientsPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;multi-client;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixLegacyServerMultipleClientsPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;multi-client;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(UnixLegacyServerClientPayloadExchangeTest UnixLegacyServerClientPayloadExchangeTest.cpp)
snodec_add_test(UnixLegacyServerClientDisconnectLifecycleTest UnixLegacyServerClientDisconnectLifecycleTest.cpp)
snodec_add_test(UnixLegacyClientConnectFailureTest UnixLegacyClientConnectFailureTest.cpp)

target_link_libraries(UnixLegacyServerClientPayloadExchangeTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_link_libraries(UnixLegacyServerClientDisconnectLifecycleTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_link_libraries(UnixLegacyClientConnectFailureTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_compile_features(UnixLegacyServerClientPayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyServerClientDisconnectLifecycleTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyClientConnectFailureTest PRIVATE cxx_std_20)

set_tests_properties(UnixLegacyServerClientPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixLegacyServerClientDisconnectLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;disconnect"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixLegacyClientConnectFailureTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;connect-failure"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(InetLegacyServerClientPayloadExchangeTest InetLegacyServerClientPayloadExchangeTest.cpp)
snodec_add_test(InetLegacyServerClientDisconnectLifecycleTest InetLegacyServerClientDisconnectLifecycleTest.cpp)
snodec_add_test(InetLegacyClientConnectFailureTest InetLegacyClientConnectFailureTest.cpp)

target_link_libraries(InetLegacyServerClientPayloadExchangeTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(InetLegacyServerClientDisconnectLifecycleTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(InetLegacyClientConnectFailureTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_compile_features(InetLegacyServerClientPayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(InetLegacyServerClientDisconnectLifecycleTest PRIVATE cxx_std_20)
target_compile_features(InetLegacyClientConnectFailureTest PRIVATE cxx_std_20)

set_tests_properties(InetLegacyServerClientPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetLegacyServerClientDisconnectLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;disconnect"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetLegacyClientConnectFailureTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;connect-failure"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(Inet6LegacyServerClientPayloadExchangeTest Inet6LegacyServerClientPayloadExchangeTest.cpp)
snodec_add_test(Inet6LegacyServerClientDisconnectLifecycleTest Inet6LegacyServerClientDisconnectLifecycleTest.cpp)
snodec_add_test(Inet6LegacyClientConnectFailureTest Inet6LegacyClientConnectFailureTest.cpp)

target_link_libraries(Inet6LegacyServerClientPayloadExchangeTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(Inet6LegacyServerClientDisconnectLifecycleTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(Inet6LegacyClientConnectFailureTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_compile_features(Inet6LegacyServerClientPayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyServerClientDisconnectLifecycleTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyClientConnectFailureTest PRIVATE cxx_std_20)

set_tests_properties(Inet6LegacyServerClientPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6LegacyServerClientDisconnectLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;disconnect"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6LegacyClientConnectFailureTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;connect-failure"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


snodec_add_test(InetLegacyServerEffectiveListenAddressTest InetLegacyServerEffectiveListenAddressTest.cpp)
snodec_add_test(InetPhysicalServerEffectiveBindAddressTest InetPhysicalServerEffectiveBindAddressTest.cpp)

target_link_libraries(InetLegacyServerEffectiveListenAddressTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(InetPhysicalServerEffectiveBindAddressTest PRIVATE snodec-test-support snodec::net-in-phy-stream)
target_compile_features(InetLegacyServerEffectiveListenAddressTest PRIVATE cxx_std_20)
target_compile_features(InetPhysicalServerEffectiveBindAddressTest PRIVATE cxx_std_20)

set_tests_properties(InetLegacyServerEffectiveListenAddressTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;listen;effective-address"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetPhysicalServerEffectiveBindAddressTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;listen;effective-address"
                     TIMEOUT 5)

snodec_add_test(InetLegacyServerClientLargePayloadExchangeTest InetLegacyServerClientLargePayloadExchangeTest.cpp)
snodec_add_test(InetLegacyServerClientFramedPayloadExchangeTest InetLegacyServerClientFramedPayloadExchangeTest.cpp)
snodec_add_test(Inet6LegacyServerClientFramedPayloadExchangeTest Inet6LegacyServerClientFramedPayloadExchangeTest.cpp)
snodec_add_test(UnixLegacyServerClientFramedPayloadExchangeTest UnixLegacyServerClientFramedPayloadExchangeTest.cpp)
snodec_add_test(Inet6LegacyServerClientLargePayloadExchangeTest Inet6LegacyServerClientLargePayloadExchangeTest.cpp)
snodec_add_test(UnixLegacyServerClientLargePayloadExchangeTest UnixLegacyServerClientLargePayloadExchangeTest.cpp)
snodec_add_test(InetLegacyServerControlledCloseLifecycleTest InetLegacyServerControlledCloseLifecycleTest.cpp)
snodec_add_test(Inet6LegacyServerControlledCloseLifecycleTest Inet6LegacyServerControlledCloseLifecycleTest.cpp)
snodec_add_test(UnixLegacyServerControlledCloseLifecycleTest UnixLegacyServerControlledCloseLifecycleTest.cpp)
snodec_add_test(InetLegacyServerMultipleClientsDisconnectLifecycleTest InetLegacyServerMultipleClientsDisconnectLifecycleTest.cpp)
snodec_add_test(Inet6LegacyServerMultipleClientsDisconnectLifecycleTest Inet6LegacyServerMultipleClientsDisconnectLifecycleTest.cpp)
snodec_add_test(UnixLegacyServerMultipleClientsDisconnectLifecycleTest UnixLegacyServerMultipleClientsDisconnectLifecycleTest.cpp)

target_link_libraries(InetLegacyServerClientLargePayloadExchangeTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(InetLegacyServerClientFramedPayloadExchangeTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(Inet6LegacyServerClientFramedPayloadExchangeTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(UnixLegacyServerClientFramedPayloadExchangeTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_link_libraries(Inet6LegacyServerClientLargePayloadExchangeTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(UnixLegacyServerClientLargePayloadExchangeTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_link_libraries(InetLegacyServerControlledCloseLifecycleTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(Inet6LegacyServerControlledCloseLifecycleTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(UnixLegacyServerControlledCloseLifecycleTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)
target_link_libraries(InetLegacyServerMultipleClientsDisconnectLifecycleTest PRIVATE snodec-test-support snodec::net-in-stream-legacy)
target_link_libraries(Inet6LegacyServerMultipleClientsDisconnectLifecycleTest PRIVATE snodec-test-support snodec::net-in6-stream-legacy)
target_link_libraries(UnixLegacyServerMultipleClientsDisconnectLifecycleTest PRIVATE snodec-test-support snodec::net-un-stream-legacy)

target_compile_features(InetLegacyServerClientLargePayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(InetLegacyServerClientFramedPayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyServerClientFramedPayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyServerClientFramedPayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyServerClientLargePayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyServerClientLargePayloadExchangeTest PRIVATE cxx_std_20)
target_compile_features(InetLegacyServerControlledCloseLifecycleTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyServerControlledCloseLifecycleTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyServerControlledCloseLifecycleTest PRIVATE cxx_std_20)
target_compile_features(InetLegacyServerMultipleClientsDisconnectLifecycleTest PRIVATE cxx_std_20)
target_compile_features(Inet6LegacyServerMultipleClientsDisconnectLifecycleTest PRIVATE cxx_std_20)
target_compile_features(UnixLegacyServerMultipleClientsDisconnectLifecycleTest PRIVATE cxx_std_20)

set_tests_properties(InetLegacyServerClientLargePayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;large-payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(InetLegacyServerClientFramedPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;framed-payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(Inet6LegacyServerClientFramedPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;framed-payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(UnixLegacyServerClientFramedPayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;framed-payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(Inet6LegacyServerClientLargePayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;large-payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(UnixLegacyServerClientLargePayloadExchangeTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;large-payload"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(InetLegacyServerControlledCloseLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;server-close"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(Inet6LegacyServerControlledCloseLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;server-close"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(UnixLegacyServerControlledCloseLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;server-close"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(InetLegacyServerMultipleClientsDisconnectLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv4;multi-client;disconnect"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(Inet6LegacyServerMultipleClientsDisconnectLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;ipv6;multi-client;disconnect"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
set_tests_properties(UnixLegacyServerMultipleClientsDisconnectLifecycleTest PROPERTIES
                     LABELS "component;net;stream;legacy;unix;multi-client;disconnect"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
