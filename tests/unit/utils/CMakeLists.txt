snodec_add_test(
    ConfigFormatterCommentMetadataTest ConfigFormatterCommentMetadataTest.cpp
)
target_link_libraries(
    ConfigFormatterCommentMetadataTest PRIVATE snodec-test-support
                                               snodec::utils
)
set_tests_properties(
    ConfigFormatterCommentMetadataTest
    PROPERTIES LABELS "unit;utils;config;format" TIMEOUT 5
)

snodec_add_test(ConfigRequirementStateTest ConfigRequirementStateTest.cpp)
target_link_libraries(
    ConfigRequirementStateTest PRIVATE snodec-test-support snodec::utils
                                       snodec::net
)
set_tests_properties(
    ConfigRequirementStateTest PROPERTIES LABELS "unit;utils;config" TIMEOUT 5
)

snodec_add_test(HexDumpPresentationTest HexDumpPresentationTest.cpp)
target_include_directories(
    HexDumpPresentationTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_compile_features(HexDumpPresentationTest PRIVATE cxx_std_20)
target_link_libraries(
    HexDumpPresentationTest PRIVATE snodec-test-support snodec::utils
                                    snodec::logger
)
set_tests_properties(
    HexDumpPresentationTest PROPERTIES LABELS "unit;utils;hexdump" TIMEOUT 5
)

add_executable(
    ConfigDirectoryInitializationTest ConfigDirectoryInitializationTest.cpp
)
target_link_libraries(
    ConfigDirectoryInitializationTest PRIVATE snodec-test-support snodec::utils
                                              ${CMAKE_DL_LIBS}
)
target_compile_features(ConfigDirectoryInitializationTest PRIVATE cxx_std_20)
target_link_options(
    ConfigDirectoryInitializationTest PRIVATE -Wl,--export-dynamic
)
foreach(
    scenario
    fresh
    existing
    race-config
    race-state
    race-runtime
    file-config
    file-state
    file-runtime
)
    add_test(NAME ConfigDirectoryInitializationTest-${scenario}
             COMMAND ConfigDirectoryInitializationTest ${scenario}
    )
    set_tests_properties(
        ConfigDirectoryInitializationTest-${scenario}
        PROPERTIES LABELS "unit;utils;config;filesystem" SKIP_RETURN_CODE 77
                   TIMEOUT 5
    )
endforeach()
