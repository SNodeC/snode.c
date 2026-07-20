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

add_executable(CodexStdioProtocolFlowTest CodexStdioProtocolFlowTest.cpp)
target_link_libraries(
    CodexStdioProtocolFlowTest PRIVATE snodec-test-support
                                       snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexStdioProtocolFlowTest PRIVATE cxx_std_20)
target_compile_definitions(
    CodexStdioProtocolFlowTest
    PRIVATE CODEX_FAKE_APP_SERVER="$<TARGET_FILE:CodexFakeAppServer>"
)
add_dependencies(CodexStdioProtocolFlowTest CodexFakeAppServer)

foreach(scenario IN ITEMS flow typed-flow exit-restart)
    string(REPLACE "-" "_" scenario_name ${scenario})
    set(protocol_flow_labels "component;ai;openai;codex;stdio;protocol")
    if(scenario STREQUAL "typed-flow")
        list(APPEND protocol_flow_labels typed)
    endif()
    add_test(NAME CodexStdioProtocolFlow_${scenario_name}
             COMMAND CodexStdioProtocolFlowTest ${scenario}
    )
    set_tests_properties(
        CodexStdioProtocolFlow_${scenario_name}
        PROPERTIES LABELS "${protocol_flow_labels}" SKIP_RETURN_CODE 77 TIMEOUT
                   10
    )
endforeach()

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

snodec_add_test(CodexProtocolEngineTest CodexProtocolEngineTest.cpp)
target_link_libraries(
    CodexProtocolEngineTest PRIVATE snodec-test-support snodec::ai-openai-codex
                                    snodec::core
)
target_compile_features(CodexProtocolEngineTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexProtocolEngineTest
    PROPERTIES LABELS "component;ai;openai;codex;protocol;engine"
               SKIP_RETURN_CODE 77 TIMEOUT 30
)

foreach(
    typed_test IN
    ITEMS CodexTypedThreadCodecTest CodexTypedTurnCodecTest
          CodexTypedItemDecoderTest CodexTypedEventDecoderTest
          CodexTypedServerRequestDecoderTest
)
    snodec_add_test(${typed_test} ${typed_test}.cpp)
    target_link_libraries(
        ${typed_test} PRIVATE snodec-test-support snodec::ai-openai-codex
    )
    target_compile_features(${typed_test} PRIVATE cxx_std_20)
    set_tests_properties(
        ${typed_test} PROPERTIES LABELS "component;ai;openai;codex;typed;codec"
                                 SKIP_RETURN_CODE 77 TIMEOUT 5
    )
endforeach()

snodec_add_test(CodexTypedProtocolEngineTest CodexTypedProtocolEngineTest.cpp)
target_link_libraries(
    CodexTypedProtocolEngineTest PRIVATE snodec-test-support
                                         snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexTypedProtocolEngineTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexTypedProtocolEngineTest
    PROPERTIES LABELS "component;ai;openai;codex;typed;protocol;engine"
               SKIP_RETURN_CODE 77 TIMEOUT 30
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

add_executable(CodexGlobalShutdownTest CodexGlobalShutdownTest.cpp)
target_link_libraries(
    CodexGlobalShutdownTest PRIVATE snodec-test-support snodec::ai-openai-codex
                                    snodec::core
)
target_compile_features(CodexGlobalShutdownTest PRIVATE cxx_std_20)
target_compile_definitions(
    CodexGlobalShutdownTest
    PRIVATE CODEX_FAKE_APP_SERVER="$<TARGET_FILE:CodexFakeAppServer>"
)
add_dependencies(CodexGlobalShutdownTest CodexFakeAppServer)

foreach(scenario IN ITEMS forced-polling-normal forced-polling-ignore-sigterm
                          ignore-sigterm destroy-client
)
    string(REPLACE "-" "_" scenario_name ${scenario})
    add_test(NAME CodexGlobalShutdown_${scenario_name}
             COMMAND CodexGlobalShutdownTest ${scenario}
    )
    set_tests_properties(
        CodexGlobalShutdown_${scenario_name}
        PROPERTIES LABELS "component;ai;openai;codex;stdio;shutdown"
                   SKIP_RETURN_CODE 77 TIMEOUT 5
    )
endforeach()

snodec_add_test(CodexShutdownAdmissionTest CodexShutdownAdmissionTest.cpp)
target_link_libraries(
    CodexShutdownAdmissionTest PRIVATE snodec-test-support
                                       snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexShutdownAdmissionTest PRIVATE cxx_std_20)
target_compile_definitions(
    CodexShutdownAdmissionTest
    PRIVATE CODEX_FAKE_APP_SERVER="$<TARGET_FILE:CodexFakeAppServer>"
)
add_dependencies(CodexShutdownAdmissionTest CodexFakeAppServer)
set_tests_properties(
    CodexShutdownAdmissionTest
    PROPERTIES LABELS "component;ai;openai;codex;stdio;shutdown"
               SKIP_RETURN_CODE 77 TIMEOUT 5
)

add_executable(CodexDescriptorCollisionTest CodexDescriptorCollisionTest.cpp)
target_link_libraries(
    CodexDescriptorCollisionTest PRIVATE snodec-test-support
                                         snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexDescriptorCollisionTest PRIVATE cxx_std_20)
target_compile_definitions(
    CodexDescriptorCollisionTest
    PRIVATE CODEX_FAKE_APP_SERVER="$<TARGET_FILE:CodexFakeAppServer>"
)
add_dependencies(CodexDescriptorCollisionTest CodexFakeAppServer)

foreach(scenario IN ITEMS stdin stdout stderr all)
    add_test(NAME CodexDescriptorCollision_${scenario}
             COMMAND CodexDescriptorCollisionTest ${scenario}
    )
    set_tests_properties(
        CodexDescriptorCollision_${scenario}
        PROPERTIES LABELS "component;ai;openai;codex;stdio;descriptor"
                   SKIP_RETURN_CODE 77 TIMEOUT 5
    )
endforeach()

foreach(
    scenario IN
    ITEMS exit-before-initialize
          close-stdout
          initialization-error
          malformed-json
          launch-failure
          post-spawn-setup-failure
          registration-lifecycle-failure
          registration-pidfd-fallback
          registration-stdin-failure
          registration-stdout-failure
          registration-stderr-failure
          bounded-overflow
          slow-stdin
          ignore-shutdown
          blocked-shutdown
          blocked-ignore-shutdown
          stop-starting
          stop-initializing
          stdio-framing
          close-stderr
          recover-after-failure
          forced-fallback-normal
          forced-fallback-ignore-shutdown
          forced-fallback-close-stdout
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

snodec_add_test(
    CodexTypedAppServerIntegrationTest CodexTypedAppServerIntegrationTest.cpp
)
target_link_libraries(
    CodexTypedAppServerIntegrationTest
    PRIVATE snodec-test-support snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexTypedAppServerIntegrationTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexTypedAppServerIntegrationTest
    PROPERTIES LABELS "integration;ai;openai;codex;typed;optional"
               SKIP_RETURN_CODE 77 TIMEOUT 60
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
