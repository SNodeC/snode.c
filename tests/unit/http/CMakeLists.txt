# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(HttpMessageParserTest HttpMessageParserTest.cpp)

target_link_libraries(HttpMessageParserTest PRIVATE snodec-test-support snodec::http-server snodec::http-client)

set_tests_properties(HttpMessageParserTest PROPERTIES LABELS "unit;http;message" TIMEOUT 5)
