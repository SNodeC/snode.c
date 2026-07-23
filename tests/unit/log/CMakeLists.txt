function(snodec_add_log_test test_name)
    snodec_add_test(${test_name} ${test_name}.cpp)
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR})
    target_compile_features(${test_name} PRIVATE cxx_std_20)
    set_tests_properties(${test_name} PROPERTIES LABELS "unit;log")
endfunction()

snodec_add_log_test(SemanticLoggerFormattingTest)
target_link_libraries(SemanticLoggerFormattingTest PRIVATE snodec-test-support snodec::logger)
set_property(TEST SemanticLoggerFormattingTest APPEND PROPERTY LABELS "format")

snodec_add_log_test(LogScopeOwnerTest)
target_link_libraries(LogScopeOwnerTest PRIVATE snodec-test-support snodec::logger)
set_property(TEST LogScopeOwnerTest APPEND PROPERTY LABELS "ownership")

snodec_add_log_test(SemanticLoggerScopeTest)
target_link_libraries(SemanticLoggerScopeTest PRIVATE snodec-test-support snodec::net snodec::core-socket-stream)
set_property(TEST SemanticLoggerScopeTest APPEND PROPERTY LABELS "scope;api")

snodec_add_log_test(SemanticLoggerBackendTest)
target_link_libraries(SemanticLoggerBackendTest PRIVATE snodec-test-support snodec::net snodec::core-socket-stream)
set_property(TEST SemanticLoggerBackendTest APPEND PROPERTY LABELS "backend")

snodec_add_log_test(SemanticLoggerFilteringTest)
target_link_libraries(SemanticLoggerFilteringTest PRIVATE snodec-test-support snodec::net snodec::core-socket-stream)
set_property(TEST SemanticLoggerFilteringTest APPEND PROPERTY LABELS "filter")

snodec_add_log_test(SemanticLoggerCompatibilityTest)
target_link_libraries(SemanticLoggerCompatibilityTest PRIVATE snodec-test-support snodec::logger)
set_property(TEST SemanticLoggerCompatibilityTest APPEND PROPERTY LABELS "compatibility;legacy")

snodec_add_log_test(SemanticLoggerDisabledPathTest)
target_link_libraries(SemanticLoggerDisabledPathTest PRIVATE snodec-test-support snodec::net snodec::core-socket-stream)
set_property(TEST SemanticLoggerDisabledPathTest APPEND PROPERTY LABELS "overhead")

snodec_add_log_test(SemanticLoggerCacheTest)
target_link_libraries(SemanticLoggerCacheTest PRIVATE snodec-test-support snodec::core snodec::logger)
set_property(TEST SemanticLoggerCacheTest APPEND PROPERTY LABELS "cache;overhead")

snodec_add_log_test(SemanticEndToEndOutputTest)
target_link_libraries(SemanticEndToEndOutputTest PRIVATE snodec-test-support snodec::logger)
set_property(TEST SemanticEndToEndOutputTest APPEND PROPERTY LABELS "e2e")

snodec_add_log_test(SemanticTerminalColorRoutingTest)
target_link_libraries(SemanticTerminalColorRoutingTest PRIVATE snodec-test-support snodec::logger snodec::utils)
set_property(TEST SemanticTerminalColorRoutingTest APPEND PROPERTY LABELS "routing;security")

snodec_add_log_test(SemanticCliIntegrationTest)
target_link_libraries(SemanticCliIntegrationTest PRIVATE snodec-test-support snodec::utils snodec::logger)
set_property(TEST SemanticCliIntegrationTest APPEND PROPERTY LABELS "cli;integration")
