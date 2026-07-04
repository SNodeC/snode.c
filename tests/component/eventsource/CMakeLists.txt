# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(InetSseEventSourceBasicEventTest InetSseEventSourceBasicEventTest.cpp)
snodec_add_test(InetSseEventSourceMultiEventSequenceTest InetSseEventSourceMultiEventSequenceTest.cpp)
snodec_add_test(InetSseEventSourceMultiLineDataTest InetSseEventSourceMultiLineDataTest.cpp)

target_link_libraries(InetSseEventSourceBasicEventTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetSseEventSourceMultiEventSequenceTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)
target_link_libraries(InetSseEventSourceMultiLineDataTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::net-in-stream-legacy)

target_compile_features(InetSseEventSourceBasicEventTest PRIVATE cxx_std_20)
target_compile_features(InetSseEventSourceMultiEventSequenceTest PRIVATE cxx_std_20)
target_compile_features(InetSseEventSourceMultiLineDataTest PRIVATE cxx_std_20)

set_tests_properties(InetSseEventSourceBasicEventTest PROPERTIES
                     LABELS "component;sse;eventsource;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetSseEventSourceMultiEventSequenceTest PROPERTIES
                     LABELS "component;sse;eventsource;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetSseEventSourceMultiLineDataTest PROPERTIES
                     LABELS "component;sse;eventsource;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
