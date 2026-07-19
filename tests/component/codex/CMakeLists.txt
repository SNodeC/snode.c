# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

add_executable(CodexFakeAppServer CodexFakeAppServer.cpp)
target_compile_features(CodexFakeAppServer PRIVATE cxx_std_20)

snodec_add_test(CodexStdioLifecycleTest CodexStdioLifecycleTest.cpp)
target_link_libraries(
    CodexStdioLifecycleTest PRIVATE snodec-test-support snodec::ai-openai-codex
                                    snodec::core
)
target_compile_features(CodexStdioLifecycleTest PRIVATE cxx_std_20)
target_compile_definitions(
    CodexStdioLifecycleTest
    PRIVATE CODEX_FAKE_APP_SERVER="$<TARGET_FILE:CodexFakeAppServer>"
)
add_dependencies(CodexStdioLifecycleTest CodexFakeAppServer)

snodec_add_test(CodexProtocolCodecTest CodexProtocolCodecTest.cpp)
target_link_libraries(
    CodexProtocolCodecTest PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(CodexProtocolCodecTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexProtocolCodecTest
    PROPERTIES LABELS "component;ai;openai;codex;protocol" SKIP_RETURN_CODE 77
               TIMEOUT 5
)

add_executable(CodexStdioScenarioTest CodexStdioScenarioTest.cpp)
target_link_libraries(
    CodexStdioScenarioTest PRIVATE snodec-test-support snodec::ai-openai-codex
                                   snodec::core
)
target_compile_features(CodexStdioScenarioTest PRIVATE cxx_std_20)
target_compile_definitions(
    CodexStdioScenarioTest
    PRIVATE CODEX_FAKE_APP_SERVER="$<TARGET_FILE:CodexFakeAppServer>"
)
add_dependencies(CodexStdioScenarioTest CodexFakeAppServer)

foreach(
    scenario IN
    ITEMS exit-before-initialize
          close-stdout
          initialization-error
          malformed-json
          launch-failure
          bounded-overflow
          slow-stdin
          ignore-shutdown
          stop-starting
          stop-initializing
          stdio-framing
          close-stderr
          recover-after-failure
          forced-fallback-normal
          forced-fallback-exit-before-initialize
)
    string(REPLACE "-" "_" scenario_name ${scenario})
    add_test(NAME CodexStdioScenario_${scenario_name}
             COMMAND CodexStdioScenarioTest ${scenario}
    )
    set_tests_properties(
        CodexStdioScenario_${scenario_name}
        PROPERTIES LABELS "component;ai;openai;codex;stdio" SKIP_RETURN_CODE 77
                   TIMEOUT 8
    )
endforeach()

snodec_add_test(CodexAppServerIntegrationTest CodexAppServerIntegrationTest.cpp)
target_link_libraries(
    CodexAppServerIntegrationTest PRIVATE snodec-test-support
                                          snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexAppServerIntegrationTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexAppServerIntegrationTest
    PROPERTIES LABELS "integration;ai;openai;codex;optional" SKIP_RETURN_CODE
               77 TIMEOUT 15
)

set_tests_properties(
    CodexStdioLifecycleTest
    PROPERTIES LABELS "component;ai;openai;codex;stdio;lifecycle"
               SKIP_RETURN_CODE 77 TIMEOUT 8
)
