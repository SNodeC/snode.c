# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(InetHttpServerClientGetRoundTripTest InetHttpServerClientGetRoundTripTest.cpp)
snodec_add_test(Inet6HttpServerClientGetRoundTripTest Inet6HttpServerClientGetRoundTripTest.cpp)
snodec_add_test(UnixHttpServerClientGetRoundTripTest UnixHttpServerClientGetRoundTripTest.cpp)

target_link_libraries(InetHttpServerClientGetRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(Inet6HttpServerClientGetRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in6-stream-legacy)
target_link_libraries(UnixHttpServerClientGetRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-un-stream-legacy)

target_compile_features(InetHttpServerClientGetRoundTripTest PRIVATE cxx_std_20)
target_compile_features(Inet6HttpServerClientGetRoundTripTest PRIVATE cxx_std_20)
target_compile_features(UnixHttpServerClientGetRoundTripTest PRIVATE cxx_std_20)

set_tests_properties(InetHttpServerClientGetRoundTripTest PROPERTIES
                     LABELS "component;http;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6HttpServerClientGetRoundTripTest PROPERTIES
                     LABELS "component;http;client;server;legacy;ipv6"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixHttpServerClientGetRoundTripTest PROPERTIES
                     LABELS "component;http;client;server;legacy;unix"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
