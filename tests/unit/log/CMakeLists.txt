snodec_add_test(SemanticLoggerRound2Test SemanticLoggerRound2Test.cpp)
target_include_directories(SemanticLoggerRound2Test PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(SemanticLoggerRound2Test PRIVATE snodec-test-support snodec::logger)
set_tests_properties(SemanticLoggerRound2Test PROPERTIES LABELS "unit;log;round2")

snodec_add_test(LogScopeOwnerRound3Test LogScopeOwnerRound3Test.cpp)
target_include_directories(LogScopeOwnerRound3Test PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(LogScopeOwnerRound3Test PRIVATE snodec-test-support snodec::logger)
set_tests_properties(LogScopeOwnerRound3Test PROPERTIES LABELS "unit;log;round3")
