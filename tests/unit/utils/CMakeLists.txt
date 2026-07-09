snodec_add_test(ConfigFormatterCommentMetadataTest ConfigFormatterCommentMetadataTest.cpp)
target_link_libraries(ConfigFormatterCommentMetadataTest PRIVATE snodec-test-support snodec::utils)
set_tests_properties(ConfigFormatterCommentMetadataTest PROPERTIES LABELS "unit;utils;config;format" TIMEOUT 5)

snodec_add_test(ConfigRequirementStateTest ConfigRequirementStateTest.cpp)
target_link_libraries(ConfigRequirementStateTest PRIVATE snodec-test-support snodec::utils snodec::net)
set_tests_properties(ConfigRequirementStateTest PROPERTIES LABELS "unit;utils;config" TIMEOUT 5)
