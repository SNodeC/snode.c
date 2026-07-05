snodec_add_test(SemanticLoggerRound2Test SemanticLoggerRound2Test.cpp)
target_include_directories(SemanticLoggerRound2Test PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(SemanticLoggerRound2Test PRIVATE snodec-test-support snodec::logger)
set_tests_properties(SemanticLoggerRound2Test PROPERTIES LABELS "unit;log;round2")

snodec_add_test(LogScopeOwnerRound3Test LogScopeOwnerRound3Test.cpp)
target_include_directories(LogScopeOwnerRound3Test PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(LogScopeOwnerRound3Test PRIVATE snodec-test-support snodec::logger)
set_tests_properties(LogScopeOwnerRound3Test PROPERTIES LABELS "unit;log;round3")

snodec_add_test(ProductionLogApiRound4Test ProductionLogApiRound4Test.cpp)
target_include_directories(ProductionLogApiRound4Test PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(ProductionLogApiRound4Test PRIVATE snodec-test-support snodec::net snodec::core-socket-stream)
set_tests_properties(ProductionLogApiRound4Test PROPERTIES LABELS "unit;log;round4")
