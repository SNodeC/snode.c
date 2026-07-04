# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(InetExpressRoutingMiddlewareTest InetExpressRoutingMiddlewareTest.cpp)

snodec_add_test(Inet6ExpressTransportSmokeTest Inet6ExpressTransportSmokeTest.cpp)
snodec_add_test(UnixExpressTransportSmokeTest UnixExpressTransportSmokeTest.cpp)
snodec_add_test(InetExpressMiddlewareShortCircuitTest InetExpressMiddlewareShortCircuitTest.cpp)
snodec_add_test(InetExpressRoutingEdgeBasicsTest InetExpressRoutingEdgeBasicsTest.cpp)

target_link_libraries(InetExpressRoutingMiddlewareTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)

target_link_libraries(Inet6ExpressTransportSmokeTest PRIVATE snodec-test-support snodec::http-server-express-legacy-in6 snodec::http-client snodec::net-in6-stream-legacy)
target_link_libraries(UnixExpressTransportSmokeTest PRIVATE snodec-test-support snodec::http-server-express-legacy-un snodec::http-client snodec::net-un-stream-legacy)
target_link_libraries(InetExpressMiddlewareShortCircuitTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetExpressRoutingEdgeBasicsTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)

target_compile_features(InetExpressRoutingMiddlewareTest PRIVATE cxx_std_20)

target_compile_features(Inet6ExpressTransportSmokeTest PRIVATE cxx_std_20)
target_compile_features(UnixExpressTransportSmokeTest PRIVATE cxx_std_20)
target_compile_features(InetExpressMiddlewareShortCircuitTest PRIVATE cxx_std_20)
target_compile_features(InetExpressRoutingEdgeBasicsTest PRIVATE cxx_std_20)

set_tests_properties(InetExpressRoutingMiddlewareTest PROPERTIES
                     LABELS "component;express;routing;middleware;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6ExpressTransportSmokeTest PROPERTIES
                     LABELS "component;express;routing;legacy;ipv6"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixExpressTransportSmokeTest PROPERTIES
                     LABELS "component;express;routing;legacy;unix"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetExpressMiddlewareShortCircuitTest PROPERTIES
                     LABELS "component;express;routing;middleware;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetExpressRoutingEdgeBasicsTest PROPERTIES
                     LABELS "component;express;routing;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
