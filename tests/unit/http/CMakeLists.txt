# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(HttpMessageParserTest HttpMessageParserTest.cpp)
snodec_add_test(HttpRequestFormatterRawWireTest HttpRequestFormatterRawWireTest.cpp)
snodec_add_test(HttpResponseFormatterRawWireTest HttpResponseFormatterRawWireTest.cpp)
snodec_add_test(HttpMessagePresentationTest HttpMessagePresentationTest.cpp)
snodec_add_test(HttpMessageHeaderCasingTest HttpMessageHeaderCasingTest.cpp)
snodec_add_test(HttpMessageTargetQueryEdgeTest HttpMessageTargetQueryEdgeTest.cpp)

target_link_libraries(HttpMessageParserTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpRequestFormatterRawWireTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpResponseFormatterRawWireTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpMessagePresentationTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpMessageHeaderCasingTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpMessageTargetQueryEdgeTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)

set_tests_properties(HttpMessageParserTest PROPERTIES LABELS "unit;http;message" TIMEOUT 5)
set_tests_properties(HttpRequestFormatterRawWireTest PROPERTIES LABELS "unit;http;message;format" TIMEOUT 5)
set_tests_properties(HttpResponseFormatterRawWireTest PROPERTIES LABELS "unit;http;message;format" TIMEOUT 5)
set_tests_properties(HttpMessagePresentationTest PROPERTIES LABELS "unit;http;message;format;presentation" TIMEOUT 5)
set_tests_properties(HttpMessageHeaderCasingTest PROPERTIES LABELS "unit;http;message;headers" TIMEOUT 5)
set_tests_properties(HttpMessageTargetQueryEdgeTest PROPERTIES LABELS "unit;http;message;target;query" TIMEOUT 5)

snodec_add_test(
    HttpServerUpgradeMissingRequestTest HttpServerUpgradeMissingRequestTest.cpp
)
target_include_directories(
    HttpServerUpgradeMissingRequestTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_compile_features(
    HttpServerUpgradeMissingRequestTest PRIVATE cxx_std_20
)
target_link_libraries(
    HttpServerUpgradeMissingRequestTest
    PRIVATE snodec-test-support snodec::http-server snodec::net
            snodec::core-socket-stream snodec::logger
)
set_tests_properties(
    HttpServerUpgradeMissingRequestTest
    PROPERTIES LABELS "unit;http;lifecycle;log" TIMEOUT 5
)

snodec_add_test(
    HttpStreamingSourceFailureTest HttpStreamingSourceFailureTest.cpp
)
target_include_directories(
    HttpStreamingSourceFailureTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_compile_features(HttpStreamingSourceFailureTest PRIVATE cxx_std_20)
target_link_libraries(
    HttpStreamingSourceFailureTest
    PRIVATE snodec-test-support snodec::http-server snodec::net
            snodec::core-socket-stream snodec::logger
)
set_tests_properties(
    HttpStreamingSourceFailureTest
    PROPERTIES LABELS "unit;http;streaming;lifecycle;log" TIMEOUT 5
)
