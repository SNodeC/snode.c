# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(InetExpressRoutingMiddlewareTest InetExpressRoutingMiddlewareTest.cpp)

snodec_add_test(Inet6ExpressTransportSmokeTest Inet6ExpressTransportSmokeTest.cpp)
snodec_add_test(UnixExpressTransportSmokeTest UnixExpressTransportSmokeTest.cpp)
snodec_add_test(InetExpressMiddlewareShortCircuitTest InetExpressMiddlewareShortCircuitTest.cpp)
snodec_add_test(InetExpressRoutingEdgeBasicsTest InetExpressRoutingEdgeBasicsTest.cpp)

snodec_add_test(InetExpressMountedRouterBasicsTest InetExpressMountedRouterBasicsTest.cpp)
snodec_add_test(InetExpressNestedRouterBasicsTest InetExpressNestedRouterBasicsTest.cpp)
snodec_add_test(InetExpressRouteParameterRegexTest InetExpressRouteParameterRegexTest.cpp)
snodec_add_test(InetExpressMiddlewareMountOrderTest InetExpressMiddlewareMountOrderTest.cpp)
snodec_add_test(InetExpressQueryMountInteractionTest InetExpressQueryMountInteractionTest.cpp)
snodec_add_test(InetExpressFallbackNotFoundTest InetExpressFallbackNotFoundTest.cpp)

target_link_libraries(InetExpressRoutingMiddlewareTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)

target_link_libraries(Inet6ExpressTransportSmokeTest PRIVATE snodec-test-support snodec::http-server-express-legacy-in6 snodec::http-client snodec::net-in6-stream-legacy)
target_link_libraries(UnixExpressTransportSmokeTest PRIVATE snodec-test-support snodec::http-server-express-legacy-un snodec::http-client snodec::net-un-stream-legacy)
target_link_libraries(InetExpressMiddlewareShortCircuitTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetExpressRoutingEdgeBasicsTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)

target_link_libraries(InetExpressMountedRouterBasicsTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetExpressNestedRouterBasicsTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetExpressRouteParameterRegexTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetExpressMiddlewareMountOrderTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetExpressQueryMountInteractionTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetExpressFallbackNotFoundTest PRIVATE snodec-test-support snodec::http-server-express snodec::http-client snodec::net-in-stream-legacy)

target_compile_features(InetExpressRoutingMiddlewareTest PRIVATE cxx_std_20)

target_compile_features(Inet6ExpressTransportSmokeTest PRIVATE cxx_std_20)
target_compile_features(UnixExpressTransportSmokeTest PRIVATE cxx_std_20)
target_compile_features(InetExpressMiddlewareShortCircuitTest PRIVATE cxx_std_20)
target_compile_features(InetExpressRoutingEdgeBasicsTest PRIVATE cxx_std_20)

target_compile_features(InetExpressMountedRouterBasicsTest PRIVATE cxx_std_20)
target_compile_features(InetExpressNestedRouterBasicsTest PRIVATE cxx_std_20)
target_compile_features(InetExpressRouteParameterRegexTest PRIVATE cxx_std_20)
target_compile_features(InetExpressMiddlewareMountOrderTest PRIVATE cxx_std_20)
target_compile_features(InetExpressQueryMountInteractionTest PRIVATE cxx_std_20)
target_compile_features(InetExpressFallbackNotFoundTest PRIVATE cxx_std_20)

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

set_tests_properties(InetExpressMountedRouterBasicsTest PROPERTIES
                     LABELS "component;express;routing;mounted-router;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetExpressNestedRouterBasicsTest PROPERTIES
                     LABELS "component;express;routing;nested-router;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetExpressRouteParameterRegexTest PROPERTIES
                     LABELS "component;express;routing;parameter-regex;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetExpressMiddlewareMountOrderTest PROPERTIES
                     LABELS "component;express;middleware;mount-order;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetExpressQueryMountInteractionTest PROPERTIES
                     LABELS "component;express;routing;query;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetExpressFallbackNotFoundTest PROPERTIES
                     LABELS "component;express;routing;not-found;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
