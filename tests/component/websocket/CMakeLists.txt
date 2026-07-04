# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

snodec_add_test(InetWebSocketServerClientTextEchoTest InetWebSocketServerClientTextEchoTest.cpp)
snodec_add_test(Inet6WebSocketServerClientTextEchoTest Inet6WebSocketServerClientTextEchoTest.cpp)
snodec_add_test(UnixWebSocketServerClientTextEchoTest UnixWebSocketServerClientTextEchoTest.cpp)
snodec_add_test(InetWebSocketServerClientMultiTextMessageTest InetWebSocketServerClientMultiTextMessageTest.cpp)
snodec_add_test(InetWebSocketServerClientBinaryEchoTest InetWebSocketServerClientBinaryEchoTest.cpp)
snodec_add_test(InetWebSocketServerClientCloseHandshakeTest InetWebSocketServerClientCloseHandshakeTest.cpp)
snodec_add_test(InetWebSocketServerClientPingPongTest InetWebSocketServerClientPingPongTest.cpp)
snodec_add_test(InetWebSocketServerInitiatedCloseTest InetWebSocketServerInitiatedCloseTest.cpp)
snodec_add_test(InetWebSocketUnexpectedCloseLifecycleTest InetWebSocketUnexpectedCloseLifecycleTest.cpp)

target_link_libraries(InetWebSocketServerClientTextEchoTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in-stream-legacy)
target_link_libraries(Inet6WebSocketServerClientTextEchoTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in6-stream-legacy)
target_link_libraries(UnixWebSocketServerClientTextEchoTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-un-stream-legacy)
target_link_libraries(InetWebSocketServerClientMultiTextMessageTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in-stream-legacy)
target_link_libraries(InetWebSocketServerClientBinaryEchoTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in-stream-legacy)
target_link_libraries(InetWebSocketServerClientCloseHandshakeTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in-stream-legacy)
target_link_libraries(InetWebSocketServerClientPingPongTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in-stream-legacy)
target_link_libraries(InetWebSocketServerInitiatedCloseTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in-stream-legacy)
target_link_libraries(InetWebSocketUnexpectedCloseLifecycleTest PRIVATE snodec-test-support snodec::http-server snodec::http-client snodec::websocket-server snodec::websocket-client snodec::net-in-stream-legacy)

target_compile_features(InetWebSocketServerClientTextEchoTest PRIVATE cxx_std_20)
target_compile_features(Inet6WebSocketServerClientTextEchoTest PRIVATE cxx_std_20)
target_compile_features(UnixWebSocketServerClientTextEchoTest PRIVATE cxx_std_20)
target_compile_features(InetWebSocketServerClientMultiTextMessageTest PRIVATE cxx_std_20)
target_compile_features(InetWebSocketServerClientBinaryEchoTest PRIVATE cxx_std_20)
target_compile_features(InetWebSocketServerClientCloseHandshakeTest PRIVATE cxx_std_20)
target_compile_features(InetWebSocketServerClientPingPongTest PRIVATE cxx_std_20)
target_compile_features(InetWebSocketServerInitiatedCloseTest PRIVATE cxx_std_20)
target_compile_features(InetWebSocketUnexpectedCloseLifecycleTest PRIVATE cxx_std_20)

set_tests_properties(InetWebSocketServerClientTextEchoTest PROPERTIES
                     LABELS "component;websocket;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(Inet6WebSocketServerClientTextEchoTest PROPERTIES
                     LABELS "component;websocket;client;server;legacy;ipv6"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(UnixWebSocketServerClientTextEchoTest PROPERTIES
                     LABELS "component;websocket;client;server;legacy;unix"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetWebSocketServerClientMultiTextMessageTest PROPERTIES
                     LABELS "component;websocket;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetWebSocketServerClientBinaryEchoTest PROPERTIES
                     LABELS "component;websocket;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetWebSocketServerClientCloseHandshakeTest PROPERTIES
                     LABELS "component;websocket;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)

set_tests_properties(InetWebSocketServerClientPingPongTest PROPERTIES
                     LABELS "component;websocket;client;server;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


set_tests_properties(InetWebSocketServerInitiatedCloseTest PROPERTIES
                     LABELS "component;websocket;close;server-initiated;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)


set_tests_properties(InetWebSocketUnexpectedCloseLifecycleTest PROPERTIES
                     LABELS "component;websocket;lifecycle;unexpected-close;legacy;ipv4"
                     SKIP_RETURN_CODE 77
                     TIMEOUT 5)
