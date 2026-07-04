# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(HttpMessageParserTest HttpMessageParserTest.cpp)
snodec_add_test(HttpRequestFormatterRawWireTest HttpRequestFormatterRawWireTest.cpp)
snodec_add_test(HttpResponseFormatterRawWireTest HttpResponseFormatterRawWireTest.cpp)
snodec_add_test(HttpMessageHeaderCasingTest HttpMessageHeaderCasingTest.cpp)
snodec_add_test(HttpMessageTargetQueryEdgeTest HttpMessageTargetQueryEdgeTest.cpp)

target_link_libraries(HttpMessageParserTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpRequestFormatterRawWireTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpResponseFormatterRawWireTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpMessageHeaderCasingTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)
target_link_libraries(HttpMessageTargetQueryEdgeTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)

set_tests_properties(HttpMessageParserTest PROPERTIES LABELS "unit;http;message" TIMEOUT 5)
set_tests_properties(HttpRequestFormatterRawWireTest PROPERTIES LABELS "unit;http;message;format" TIMEOUT 5)
set_tests_properties(HttpResponseFormatterRawWireTest PROPERTIES LABELS "unit;http;message;format" TIMEOUT 5)
set_tests_properties(HttpMessageHeaderCasingTest PROPERTIES LABELS "unit;http;message;headers" TIMEOUT 5)
set_tests_properties(HttpMessageTargetQueryEdgeTest PROPERTIES LABELS "unit;http;message;target;query" TIMEOUT 5)
