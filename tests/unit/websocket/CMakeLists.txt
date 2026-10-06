snodec_add_test(
    WebSocketReceiverValidationTest WebSocketReceiverValidationTest.cpp
)
target_link_libraries(
    WebSocketReceiverValidationTest PRIVATE snodec-test-support
                                            snodec::websocket
)
target_compile_features(WebSocketReceiverValidationTest PRIVATE cxx_std_20)
set_tests_properties(
    WebSocketReceiverValidationTest PROPERTIES LABELS
                                               "unit;websocket;validation"
)

snodec_add_test(WebSocketLimitCloseFrameTest WebSocketLimitCloseFrameTest.cpp)
target_include_directories(
    WebSocketLimitCloseFrameTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_link_libraries(
    WebSocketLimitCloseFrameTest
    PRIVATE OpenSSL::SSL snodec-test-support snodec::websocket snodec::http
            snodec::net-in-stream-legacy
)
set_tests_properties(WebSocketLimitCloseFrameTest PROPERTIES TIMEOUT 20)
