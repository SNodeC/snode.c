# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(InetHttpServerClientGetRoundTripTest InetHttpServerClientGetRoundTripTest.cpp)
snodec_add_test(Inet6HttpServerClientGetRoundTripTest Inet6HttpServerClientGetRoundTripTest.cpp)
snodec_add_test(UnixHttpServerClientGetRoundTripTest UnixHttpServerClientGetRoundTripTest.cpp)

snodec_add_test(InetHttpServerClientPostBodyRoundTripTest InetHttpServerClientPostBodyRoundTripTest.cpp)
snodec_add_test(Inet6HttpServerClientPostBodyRoundTripTest Inet6HttpServerClientPostBodyRoundTripTest.cpp)
snodec_add_test(UnixHttpServerClientPostBodyRoundTripTest UnixHttpServerClientPostBodyRoundTripTest.cpp)
snodec_add_test(InetHttpServerClientStatusVariantTest InetHttpServerClientStatusVariantTest.cpp)
snodec_add_test(Inet6HttpServerClientStatusVariantTest Inet6HttpServerClientStatusVariantTest.cpp)
snodec_add_test(UnixHttpServerClientStatusVariantTest UnixHttpServerClientStatusVariantTest.cpp)

target_link_libraries(InetHttpServerClientGetRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(Inet6HttpServerClientGetRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in6-stream-legacy)
target_link_libraries(UnixHttpServerClientGetRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-un-stream-legacy)

target_link_libraries(InetHttpServerClientPostBodyRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(Inet6HttpServerClientPostBodyRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in6-stream-legacy)
target_link_libraries(UnixHttpServerClientPostBodyRoundTripTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-un-stream-legacy)
target_link_libraries(InetHttpServerClientStatusVariantTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(Inet6HttpServerClientStatusVariantTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in6-stream-legacy)
target_link_libraries(UnixHttpServerClientStatusVariantTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-un-stream-legacy)

target_compile_features(InetHttpServerClientGetRoundTripTest PRIVATE cxx_std_20)
target_compile_features(Inet6HttpServerClientGetRoundTripTest PRIVATE cxx_std_20)
target_compile_features(UnixHttpServerClientGetRoundTripTest PRIVATE cxx_std_20)

target_compile_features(InetHttpServerClientPostBodyRoundTripTest PRIVATE cxx_std_20)
target_compile_features(Inet6HttpServerClientPostBodyRoundTripTest PRIVATE cxx_std_20)
target_compile_features(UnixHttpServerClientPostBodyRoundTripTest PRIVATE cxx_std_20)
target_compile_features(InetHttpServerClientStatusVariantTest PRIVATE cxx_std_20)
target_compile_features(Inet6HttpServerClientStatusVariantTest PRIVATE cxx_std_20)
target_compile_features(UnixHttpServerClientStatusVariantTest PRIVATE cxx_std_20)

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


set_tests_properties(InetHttpServerClientPostBodyRoundTripTest PROPERTIES
                     LABELS "component;http;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6HttpServerClientPostBodyRoundTripTest PROPERTIES
                     LABELS "component;http;client;server;legacy;ipv6"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixHttpServerClientPostBodyRoundTripTest PROPERTIES
                     LABELS "component;http;client;server;legacy;unix"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetHttpServerClientStatusVariantTest PROPERTIES
                     LABELS "component;http;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6HttpServerClientStatusVariantTest PROPERTIES
                     LABELS "component;http;client;server;legacy;ipv6"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixHttpServerClientStatusVariantTest PROPERTIES
                     LABELS "component;http;client;server;legacy;unix"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

snodec_add_test(InetHttpServerClientRepeatedRequestTest InetHttpServerClientRepeatedRequestTest.cpp)
snodec_add_test(InetHttpServerClientDisconnectLifecycleTest InetHttpServerClientDisconnectLifecycleTest.cpp)
snodec_add_test(InetHttpClientPrematureServerCloseTest InetHttpClientPrematureServerCloseTest.cpp)
snodec_add_test(InetHttpServerClientLargeResponseBodyTest InetHttpServerClientLargeResponseBodyTest.cpp)
snodec_add_test(InetHttpServerClientLargeRequestBodyTest InetHttpServerClientLargeRequestBodyTest.cpp)
snodec_add_test(InetHttpServerClientFragmentedResponseTest InetHttpServerClientFragmentedResponseTest.cpp)
snodec_add_test(InetHttpServerClientFragmentedRequestTest InetHttpServerClientFragmentedRequestTest.cpp)

target_link_libraries(InetHttpServerClientRepeatedRequestTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetHttpServerClientDisconnectLifecycleTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetHttpClientPrematureServerCloseTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetHttpServerClientLargeResponseBodyTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetHttpServerClientLargeRequestBodyTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetHttpServerClientFragmentedResponseTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetHttpServerClientFragmentedRequestTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)

target_compile_features(InetHttpServerClientRepeatedRequestTest PRIVATE cxx_std_20)
target_compile_features(InetHttpServerClientDisconnectLifecycleTest PRIVATE cxx_std_20)
target_compile_features(InetHttpClientPrematureServerCloseTest PRIVATE cxx_std_20)
target_compile_features(InetHttpServerClientLargeResponseBodyTest PRIVATE cxx_std_20)
target_compile_features(InetHttpServerClientLargeRequestBodyTest PRIVATE cxx_std_20)
target_compile_features(InetHttpServerClientFragmentedResponseTest PRIVATE cxx_std_20)
target_compile_features(InetHttpServerClientFragmentedRequestTest PRIVATE cxx_std_20)

set_tests_properties(InetHttpServerClientRepeatedRequestTest
                     InetHttpServerClientDisconnectLifecycleTest
                     InetHttpClientPrematureServerCloseTest
                     InetHttpServerClientLargeResponseBodyTest
                     InetHttpServerClientLargeRequestBodyTest
                     InetHttpServerClientFragmentedResponseTest
                     InetHttpServerClientFragmentedRequestTest PROPERTIES
                     LABELS "component;http;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
