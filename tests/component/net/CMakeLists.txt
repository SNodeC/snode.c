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
