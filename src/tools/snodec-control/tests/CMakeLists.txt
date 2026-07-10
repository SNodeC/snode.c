cmake_minimum_required(VERSION 3.18)

add_library(snodec-control-test-support STATIC TestResult.cpp TestResult.h)
target_include_directories(
    snodec-control-test-support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)

function(snodec_control_add_test target)
    add_executable(${target} ${ARGN})
    target_link_libraries(
        ${target} PRIVATE snodec-control-test-support snodec-control-core
                          snodec-control-ui-logic
    )
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES TIMEOUT 10)
endfunction()

snodec_control_add_test(
    SnodecControlConfigParserTest SnodecControlConfigParserTest.cpp
)
snodec_control_add_test(SnodecControlMetadataTest SnodecControlMetadataTest.cpp)
snodec_control_add_test(
    SnodecControlModelMetadataEnrichmentTest
    SnodecControlModelMetadataEnrichmentTest.cpp
)
snodec_control_add_test(
    SnodecControlSniRegressionTest SnodecControlSniRegressionTest.cpp
)
snodec_control_add_test(
    SnodecControlJsonWriterTest SnodecControlJsonWriterTest.cpp
)
snodec_control_add_test(
    SnodecControlMaterializerTest SnodecControlMaterializerTest.cpp
)
snodec_control_add_test(
    SnodecControlConfigEditorTest SnodecControlConfigEditorTest.cpp
)
snodec_control_add_test(
    SnodecControlCommandBuilderTest SnodecControlCommandBuilderTest.cpp
)
snodec_control_add_test(SnodecControlUiTreeTest SnodecControlUiTreeTest.cpp)
snodec_control_add_test(
    SnodecControlUiTreeMetadataTest SnodecControlUiTreeMetadataTest.cpp
)
snodec_control_add_test(SnodecControlUiStateTest SnodecControlUiStateTest.cpp)
snodec_control_add_test(
    SnodecControlLineEditorTest SnodecControlLineEditorTest.cpp
)
snodec_control_add_test(
    SnodecControlRenderUtilTest SnodecControlRenderUtilTest.cpp
)
snodec_control_add_test(
    SnodecControlProcessRunnerTest SnodecControlProcessRunnerTest.cpp
)

# Only this test needs the actual Ui.h entry point (isUiAvailable()/runInteractiveUi()), to verify
# whichever of CursesUi.cpp/UiUnavailable.cpp this build picked behaves per its documented contract.
snodec_control_add_test(
    SnodecControlUiAvailabilityTest SnodecControlUiAvailabilityTest.cpp
)
target_link_libraries(
    SnodecControlUiAvailabilityTest PRIVATE snodec-control-ui
)
