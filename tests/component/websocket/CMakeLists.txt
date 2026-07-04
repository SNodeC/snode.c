# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(InetWebSocketServerClientTextEchoTest InetWebSocketServerClientTextEchoTest.cpp)

target_link_libraries(InetWebSocketServerClientTextEchoTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in-stream-legacy)

target_compile_features(InetWebSocketServerClientTextEchoTest PRIVATE cxx_std_20)

set_tests_properties(InetWebSocketServerClientTextEchoTest PROPERTIES
                     LABELS "component;websocket;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
