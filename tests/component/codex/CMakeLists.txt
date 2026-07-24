# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

set(CODEX_PHASE_A0_SCHEMA_ROOT
    ${PROJECT_SOURCE_DIR}/tools/codex/app-server-schema/0.144.6
)
set(CODEX_PHASE_A0_PROVENANCE
    ${CODEX_PHASE_A0_SCHEMA_ROOT}/PROVENANCE.json
)
set(CODEX_PHASE_A0_SURFACE_MANIFEST
    ${PROJECT_SOURCE_DIR}/tools/codex/app-server-surface/0.144.6.json
)
set(CODEX_PHASE_A0_SURFACE_TOOL
    ${PROJECT_SOURCE_DIR}/tools/codex/app_server_surface.py
)
set(CODEX_PHASE_A0_TRANSCRIPT_FIXTURES
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/app-server-0.144.6
)
set(CODEX_PHASE_A0_REGISTRY
    ${PROJECT_SOURCE_DIR}/src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc
)
set(CODEX_PHASE_A0_COVERAGE_DOC
    ${PROJECT_SOURCE_DIR}/docs/ai/openai/codex/app-server-api-coverage.md
)
set(CODEX_PHASE_A0_SECURITY_DOC
    ${PROJECT_SOURCE_DIR}/docs/ai/openai/codex/app-server-security-decisions.md
)
set(
    CODEX_PHASE_A0_PYTHON_ARGUMENTS
    --tool
    ${CODEX_PHASE_A0_SURFACE_TOOL}
    --schema-root
    ${CODEX_PHASE_A0_SCHEMA_ROOT}
    --provenance
    ${CODEX_PHASE_A0_PROVENANCE}
    --manifest
    ${CODEX_PHASE_A0_SURFACE_MANIFEST}
    --fixtures
    ${CODEX_PHASE_A0_TRANSCRIPT_FIXTURES}
    --registry
    ${CODEX_PHASE_A0_REGISTRY}
    --coverage-doc
    ${CODEX_PHASE_A0_COVERAGE_DOC}
    --security-doc
    ${CODEX_PHASE_A0_SECURITY_DOC}
)

add_test(
    NAME CodexVendoredAppServerSchemaTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexAppServerSurfaceToolTest.py vendored
        ${CODEX_PHASE_A0_PYTHON_ARGUMENTS}
)
add_test(
    NAME CodexAppServerSurfaceExtractionTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexAppServerSurfaceToolTest.py extraction
        ${CODEX_PHASE_A0_PYTHON_ARGUMENTS}
)
add_test(
    NAME CodexAppServerGeneratedArtifactsGuardTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexAppServerSurfaceToolTest.py artifacts
        ${CODEX_PHASE_A0_PYTHON_ARGUMENTS}
)
add_test(
    NAME CodexSchemaTranscriptFixtureValidationTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexAppServerSurfaceToolTest.py fixtures
        ${CODEX_PHASE_A0_PYTHON_ARGUMENTS}
)
set_tests_properties(
    CodexVendoredAppServerSchemaTest
    CodexAppServerSurfaceExtractionTest
    CodexAppServerGeneratedArtifactsGuardTest
    CodexSchemaTranscriptFixtureValidationTest
    PROPERTIES LABELS "component;ai;openai;codex;phase-a0;schema" TIMEOUT 30
)

add_test(
    NAME CodexAppServerOperationContractsTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexAppServerContractsToolTest.py
        --tool
        ${PROJECT_SOURCE_DIR}/tools/codex/app_server_contracts.py
        --source-root
        ${PROJECT_SOURCE_DIR}/tools/codex/app-server-protocol-source/0.144.6
        --schema-root
        ${CODEX_PHASE_A0_SCHEMA_ROOT}
        --manifest
        ${CODEX_PHASE_A0_SURFACE_MANIFEST}
        --schema-provenance
        ${CODEX_PHASE_A0_PROVENANCE}
        --evidence-root
        ${PROJECT_SOURCE_DIR}/tools/codex/app-server-evidence/0.144.6
)
set_tests_properties(
    CodexAppServerOperationContractsTest
    PROPERTIES
        LABELS "component;ai;openai;codex;phase-a1-0;schema;operation-contract"
        TIMEOUT 30
)

set(CODEX_PHASE_A1_1_EVIDENCE_ROOT
    ${PROJECT_SOURCE_DIR}/tools/codex/app-server-evidence/0.144.6
)
add_test(
    NAME CodexA11AuditToolTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexA11AuditToolTest.py
        --tool
        ${PROJECT_SOURCE_DIR}/tools/codex/app_server_a1_1.py
        --repo-root
        ${PROJECT_SOURCE_DIR}
        --manifest
        ${CODEX_PHASE_A0_SURFACE_MANIFEST}
        --registry
        ${CODEX_PHASE_A0_REGISTRY}
        --schema-root
        ${CODEX_PHASE_A0_SCHEMA_ROOT}
        --assignments
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/module-slice-assignment.json
        --reachability
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/nested-reachability.json
        --contracts
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/operation-contracts.json
        --schema-completeness
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/schema-completeness-evidence.json
        --fixture-coverage
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/fixture-coverage.json
        --production-coverage
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-operation-production-coverage.json
        --notification-production-coverage
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-notification-production-coverage.json
        --fixture-index
        ${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6/index.json
        --draft07-validator
        ${PROJECT_SOURCE_DIR}/tools/codex/draft07.py
        --start-state
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-start-state.json
        --plan-output
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-implementation-plan.json
        --closure-output
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-type-closure.json
)
set_tests_properties(
    CodexA11AuditToolTest
    PROPERTIES
        LABELS "component;ai;openai;codex;phase-a1-1;schema;audit;generated"
        TIMEOUT 60
)

add_test(
    NAME CodexA11ClosureEvidenceTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/tools/codex/app_server_a1_1_closure.py
        check
        --repo-root
        ${PROJECT_SOURCE_DIR}
)
set_tests_properties(
    CodexA11ClosureEvidenceTest
    PROPERTIES
        LABELS "component;ai;openai;codex;phase-a1-1;closure;generated;documentation"
        TIMEOUT 30
)

add_test(
    NAME CodexA11ArtifactByteIdentityTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexA11ArtifactByteIdentityTest.py
        --evidence-root
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}
)
set_tests_properties(
    CodexA11ArtifactByteIdentityTest
    PROPERTIES
        LABELS
        "component;ai;openai;codex;phase-a1-1;phase-a1-2;compatibility;generated"
        TIMEOUT 5
)

set(CODEX_PHASE_A1_2_EVIDENCE_ROOT
    ${PROJECT_SOURCE_DIR}/tools/codex/app-server-evidence/0.144.6
)
add_test(
    NAME CodexA12AuditToolTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexA12AuditToolTest.py
        --tool
        ${PROJECT_SOURCE_DIR}/tools/codex/app_server_a1_2.py
        --repo-root
        ${PROJECT_SOURCE_DIR}
        --manifest
        ${CODEX_PHASE_A0_SURFACE_MANIFEST}
        --registry
        ${CODEX_PHASE_A0_REGISTRY}
        --schema-root
        ${CODEX_PHASE_A0_SCHEMA_ROOT}
        --assignments
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/module-slice-assignment.json
        --reachability
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/nested-reachability.json
        --contracts
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/operation-contracts.json
        --schema-completeness
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/schema-completeness-evidence.json
        --fixture-coverage
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/fixture-coverage.json
        --fixture-index
        ${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6/index.json
        --draft07-validator
        ${PROJECT_SOURCE_DIR}/tools/codex/draft07.py
        --start-state
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/a1-2-start-state.json
        --plan-output
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/a1-2-implementation-plan.json
        --closure-output
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/a1-2-type-closure.json
)
set_tests_properties(
    CodexA12AuditToolTest
    PROPERTIES
        LABELS "component;ai;openai;codex;phase-a1-2;schema;audit;generated"
        TIMEOUT 60
)

add_test(
    NAME CodexA12DocumentationConsistencyTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexA12DocumentationConsistencyTest.py
        --documentation
        ${PROJECT_SOURCE_DIR}/docs/ai/openai/codex/a1-2-accounts-models-configuration.md
        --start-state
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/a1-2-start-state.json
        --plan
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/a1-2-implementation-plan.json
        --closure
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/a1-2-type-closure.json
)
set_tests_properties(
    CodexA12DocumentationConsistencyTest
    PROPERTIES
        LABELS
        "component;ai;openai;codex;phase-a1-2;documentation;generated"
        TIMEOUT 5
)

add_test(
    NAME CodexA12ClosureEvidenceTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexA12ClosureEvidenceTest.py
        --tool
        ${PROJECT_SOURCE_DIR}/tools/codex/app_server_a1_2_closure.py
        --repo-root
        ${PROJECT_SOURCE_DIR}
        --report
        ${CODEX_PHASE_A1_2_EVIDENCE_ROOT}/a1-2-closure-report.json
)
set_tests_properties(
    CodexA12ClosureEvidenceTest
    PROPERTIES
        DEPENDS CodexA12AuditToolTest
        LABELS
        "component;ai;openai;codex;phase-a1-2;closure;generated"
        TIMEOUT 10
)

add_test(
    NAME CodexA11OperationProductionCoverageGuardTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CODEX_PHASE_A0_SURFACE_TOOL}
        operation-production-coverage
        --manifest
        ${CODEX_PHASE_A0_SURFACE_MANIFEST}
        --evidence-root
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}
        --fixture-index
        ${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6/index.json
        --repo-root
        ${PROJECT_SOURCE_DIR}
        --output
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-operation-production-coverage.json
        --check
)
set_tests_properties(
    CodexA11OperationProductionCoverageGuardTest
    PROPERTIES
        LABELS "component;ai;openai;codex;phase-a1-1;operation;codec;corpus;generated"
        TIMEOUT 30
)

add_test(
    NAME CodexA11NotificationProductionCoverageGuardTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CODEX_PHASE_A0_SURFACE_TOOL}
        notification-production-coverage
        --manifest
        ${CODEX_PHASE_A0_SURFACE_MANIFEST}
        --evidence-root
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}
        --fixture-index
        ${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6/index.json
        --repo-root
        ${PROJECT_SOURCE_DIR}
        --output
        ${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-notification-production-coverage.json
        --check
)
set_tests_properties(
    CodexA11NotificationProductionCoverageGuardTest
    PROPERTIES
        LABELS "component;ai;openai;codex;phase-a1-1;notification;codec;corpus;generated"
        TIMEOUT 30
)

add_test(
    NAME CodexDraft07ValidatorTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexDraft07ValidatorTest.py
)
set_tests_properties(
    CodexDraft07ValidatorTest
    PROPERTIES LABELS "component;ai;openai;codex;phase-a1-0;schema;validator"
               TIMEOUT 10
)

add_test(
    NAME CodexAppServerFixtureInfrastructureTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexAppServerFixtureToolTest.py
)
set_tests_properties(
    CodexAppServerFixtureInfrastructureTest
    PROPERTIES LABELS
               "component;ai;openai;codex;phase-a1-0;schema;fixtures"
               ENVIRONMENT "PYTHONHASHSEED=1"
               TIMEOUT 300
)

add_test(
    NAME CodexFrontendProtocolByteIdentityTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexFrontendProtocolByteIdentityTest.py
        --schema
        ${PROJECT_SOURCE_DIR}/docs/ai/openai/codex/frontend-protocol-v1.schema.json
)
set_tests_properties(
    CodexFrontendProtocolByteIdentityTest
    PROPERTIES LABELS "component;ai;openai;codex;phase-a1-0;frontend;guard"
               TIMEOUT 5
)

snodec_add_test(
    CodexProtocolSurfaceRegistryTest CodexProtocolSurfaceRegistryTest.cpp
)
target_link_libraries(
    CodexProtocolSurfaceRegistryTest PRIVATE snodec-test-support
                                             snodec::ai-openai-codex
)
target_compile_features(CodexProtocolSurfaceRegistryTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexProtocolSurfaceRegistryTest
    PROPERTIES LABELS "component;ai;openai;codex;phase-a0;registry" TIMEOUT 5
)

snodec_add_test(
    CodexProtocolSurfaceCoverageGuardTest
    CodexProtocolSurfaceCoverageGuardTest.cpp
)
target_link_libraries(
    CodexProtocolSurfaceCoverageGuardTest
    PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(
    CodexProtocolSurfaceCoverageGuardTest PRIVATE cxx_std_20
)
target_compile_definitions(
    CodexProtocolSurfaceCoverageGuardTest
    PRIVATE CODEX_PHASE_A0_SURFACE_MANIFEST="${CODEX_PHASE_A0_SURFACE_MANIFEST}"
)
set_tests_properties(
    CodexProtocolSurfaceCoverageGuardTest
    PROPERTIES LABELS "component;ai;openai;codex;phase-a0;registry;guard"
               TIMEOUT 5
)

snodec_add_test(CodexSchemaTranscriptUnixTest CodexSchemaTranscriptUnixTest.cpp)
target_link_libraries(
    CodexSchemaTranscriptUnixTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-backend
)
target_compile_features(CodexSchemaTranscriptUnixTest PRIVATE cxx_std_20)
target_compile_definitions(
    CodexSchemaTranscriptUnixTest
    PRIVATE
        CODEX_PHASE_A0_TRANSCRIPT_FIXTURES="${CODEX_PHASE_A0_TRANSCRIPT_FIXTURES}"
)
set_tests_properties(
    CodexSchemaTranscriptUnixTest
    PROPERTIES LABELS "component;ai;openai;codex;phase-a0;schema;unix"
               TIMEOUT 5
)

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

snodec_add_test(CodexA11OperationWireTest CodexA11OperationWireTest.cpp)
target_link_libraries(
    CodexA11OperationWireTest
    PRIVATE snodec-test-support snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexA11OperationWireTest PRIVATE cxx_std_20)
target_compile_definitions(
    CodexA11OperationWireTest
    PRIVATE CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
)
target_compile_options(
    CodexA11OperationWireTest PRIVATE -Wno-deprecated-declarations
)
set_tests_properties(
    CodexA11OperationWireTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;operation;wire;a1-1"
        SKIP_RETURN_CODE 77
        TIMEOUT 30
)

snodec_add_test(CodexA12AccountCodecTest CodexA12AccountCodecTest.cpp)
target_link_libraries(
    CodexA12AccountCodecTest
    PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(CodexA12AccountCodecTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexA12AccountCodecTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;account;codec;a1-2;b2"
        SKIP_RETURN_CODE 77
        TIMEOUT 10
)

snodec_add_test(CodexA12AccountWireTest CodexA12AccountWireTest.cpp)
target_link_libraries(
    CodexA12AccountWireTest
    PRIVATE snodec-test-support snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexA12AccountWireTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexA12AccountWireTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;account;operation;wire;a1-2;b2"
        SKIP_RETURN_CODE 77
        TIMEOUT 20
)

snodec_add_test(CodexA12ModelCodecTest CodexA12ModelCodecTest.cpp)
target_link_libraries(
    CodexA12ModelCodecTest
    PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(CodexA12ModelCodecTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexA12ModelCodecTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;model;codec;a1-2;b3"
        SKIP_RETURN_CODE 77
        TIMEOUT 10
)

snodec_add_test(CodexA12ModelWireTest CodexA12ModelWireTest.cpp)
target_link_libraries(
    CodexA12ModelWireTest
    PRIVATE snodec-test-support snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexA12ModelWireTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexA12ModelWireTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;model;operation;wire;a1-2;b3"
        SKIP_RETURN_CODE 77
        TIMEOUT 20
)

snodec_add_test(
    CodexA12ConfigurationReadCodecTest
    CodexA12ConfigurationReadCodecTest.cpp
)
target_link_libraries(
    CodexA12ConfigurationReadCodecTest
    PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(
    CodexA12ConfigurationReadCodecTest PRIVATE cxx_std_20
)
set_tests_properties(
    CodexA12ConfigurationReadCodecTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;configuration;codec;a1-2;b4"
        SKIP_RETURN_CODE 77
        TIMEOUT 10
)

snodec_add_test(
    CodexA12ConfigurationReadWireTest
    CodexA12ConfigurationReadWireTest.cpp
)
target_link_libraries(
    CodexA12ConfigurationReadWireTest
    PRIVATE snodec-test-support snodec::ai-openai-codex snodec::core
)
target_compile_features(
    CodexA12ConfigurationReadWireTest PRIVATE cxx_std_20
)
set_tests_properties(
    CodexA12ConfigurationReadWireTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;configuration;operation;wire;a1-2;b4"
        SKIP_RETURN_CODE 77
        TIMEOUT 20
)

snodec_add_test(
    CodexA12ConfigurationMutationCodecTest
    CodexA12ConfigurationMutationCodecTest.cpp
)
target_link_libraries(
    CodexA12ConfigurationMutationCodecTest
    PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(
    CodexA12ConfigurationMutationCodecTest PRIVATE cxx_std_20
)
set_tests_properties(
    CodexA12ConfigurationMutationCodecTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;configuration;feature;codec;a1-2;b5"
        SKIP_RETURN_CODE 77
        TIMEOUT 10
)

snodec_add_test(
    CodexA12ConfigurationMutationWireTest
    CodexA12ConfigurationMutationWireTest.cpp
)
target_link_libraries(
    CodexA12ConfigurationMutationWireTest
    PRIVATE snodec-test-support snodec::ai-openai-codex snodec::core
)
target_compile_features(
    CodexA12ConfigurationMutationWireTest PRIVATE cxx_std_20
)
set_tests_properties(
    CodexA12ConfigurationMutationWireTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;configuration;feature;operation;wire;a1-2;b5"
        SKIP_RETURN_CODE 77
        TIMEOUT 20
)

snodec_add_test(
    CodexA12AuthRefreshWireTest
    CodexA12AuthRefreshWireTest.cpp
)
target_link_libraries(
    CodexA12AuthRefreshWireTest
    PRIVATE snodec-test-support snodec::ai-openai-codex snodec::core
)
target_compile_features(CodexA12AuthRefreshWireTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexA12AuthRefreshWireTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;account;server-request;wire;a1-2;b2"
        SKIP_RETURN_CODE 77
        TIMEOUT 20
)

snodec_add_test(
    CodexA11OperationResultCorpusTest
    CodexA11OperationResultCorpusTest.cpp
)
target_link_libraries(
    CodexA11OperationResultCorpusTest
    PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(
    CodexA11OperationResultCorpusTest PRIVATE cxx_std_20
)
target_compile_definitions(
    CodexA11OperationResultCorpusTest
    PRIVATE
        CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
        CODEX_A1_OPERATION_PRODUCTION_COVERAGE="${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-operation-production-coverage.json"
)
set_tests_properties(
    CodexA11OperationResultCorpusTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;operation;codec;corpus;a1-1"
        SKIP_RETURN_CODE 77
        TIMEOUT 30
)

snodec_add_test(
    CodexA11OperationAggregateValueCorpusTest
    CodexA11OperationAggregateValueCorpusTest.cpp
)
target_link_libraries(
    CodexA11OperationAggregateValueCorpusTest
    PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(
    CodexA11OperationAggregateValueCorpusTest PRIVATE cxx_std_20
)
target_compile_definitions(
    CodexA11OperationAggregateValueCorpusTest
    PRIVATE
        CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
        CODEX_A1_OPERATION_PRODUCTION_COVERAGE="${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-operation-production-coverage.json"
)
set_tests_properties(
    CodexA11OperationAggregateValueCorpusTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;operation;aggregate;value;corpus;a1-1"
        SKIP_RETURN_CODE 77
        TIMEOUT 30
)

snodec_add_test(
    CodexA11NotificationCodecTest
    CodexA11NotificationCodecTest.cpp
)
target_link_libraries(
    CodexA11NotificationCodecTest
    PRIVATE snodec-test-support snodec::ai-openai-codex
)
target_compile_features(
    CodexA11NotificationCodecTest PRIVATE cxx_std_20
)
target_compile_definitions(
    CodexA11NotificationCodecTest
    PRIVATE
        CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
        CODEX_A1_NOTIFICATION_PRODUCTION_COVERAGE="${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-notification-production-coverage.json"
)
set_tests_properties(
    CodexA11NotificationCodecTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;notification;codec;corpus;a1-1"
        SKIP_RETURN_CODE 77
        TIMEOUT 30
)

foreach(
    typed_test IN
    ITEMS CodexTypedThreadCodecTest CodexTypedTurnCodecTest
          CodexTypedItemDecoderTest CodexTypedEventDecoderTest
          CodexTypedServerRequestDecoderTest CodexErrorInfoCodecTest
          CodexConversationCodecTest CodexConversationB3NestedCodecTest
          CodexConversationB4NestedCodecTest CodexA11ItemCodecTest
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

target_compile_definitions(
    CodexErrorInfoCodecTest
    PRIVATE CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
)
target_compile_definitions(
    CodexConversationCodecTest
    PRIVATE CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
)
target_compile_definitions(
    CodexConversationB3NestedCodecTest
    PRIVATE CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
)
target_compile_definitions(
    CodexConversationB4NestedCodecTest
    PRIVATE
        CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
        CODEX_A1_OPERATION_PRODUCTION_COVERAGE="${CODEX_PHASE_A1_1_EVIDENCE_ROOT}/a1-1-operation-production-coverage.json"
)
set_tests_properties(
    CodexConversationB4NestedCodecTest
    PROPERTIES TIMEOUT 30
)
target_compile_definitions(
    CodexA11ItemCodecTest
    PRIVATE CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
)

snodec_add_test(CodexBackendReducerTest CodexBackendReducerTest.cpp)
target_link_libraries(
    CodexBackendReducerTest PRIVATE snodec-test-support
                                    snodec::ai-openai-codex-backend
)
target_compile_features(CodexBackendReducerTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexBackendReducerTest
    PROPERTIES LABELS "component;ai;openai;codex;backend;reducer"
               SKIP_RETURN_CODE 77 TIMEOUT 10
)

snodec_add_test(
    CodexThreadItemBackendParityTest
    CodexThreadItemBackendParityTest.cpp
)
target_link_libraries(
    CodexThreadItemBackendParityTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-backend
)
target_compile_features(
    CodexThreadItemBackendParityTest PRIVATE cxx_std_20
)
target_compile_definitions(
    CodexThreadItemBackendParityTest
    PRIVATE CODEX_A1_FIXTURE_ROOT="${PROJECT_SOURCE_DIR}/tools/codex/app-server-fixtures/0.144.6"
)
set_tests_properties(
    CodexThreadItemBackendParityTest
    PROPERTIES LABELS "component;ai;openai;codex;typed;backend;reducer;parity"
               SKIP_RETURN_CODE 77 TIMEOUT 10
)

snodec_add_test(
    CodexA11NotificationBackendPreservationTest
    CodexA11NotificationBackendPreservationTest.cpp
)
target_link_libraries(
    CodexA11NotificationBackendPreservationTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-backend
)
target_compile_features(
    CodexA11NotificationBackendPreservationTest PRIVATE cxx_std_20
)
set_tests_properties(
    CodexA11NotificationBackendPreservationTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;backend;reducer;preservation;a1-1"
        SKIP_RETURN_CODE 77
        TIMEOUT 10
)

snodec_add_test(
    CodexA12AccountBackendPreservationTest
    CodexA12AccountBackendPreservationTest.cpp
)
target_link_libraries(
    CodexA12AccountBackendPreservationTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-frontend snodec::core
)
target_compile_features(
    CodexA12AccountBackendPreservationTest PRIVATE cxx_std_20
)
set_tests_properties(
    CodexA12AccountBackendPreservationTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;account;backend;reducer;preservation;a1-2;b2"
        SKIP_RETURN_CODE 77
        TIMEOUT 10
)

snodec_add_test(
    CodexA12ModelBackendCompatibilityTest
    CodexA12ModelBackendCompatibilityTest.cpp
)
target_link_libraries(
    CodexA12ModelBackendCompatibilityTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-frontend snodec::core
)
target_compile_features(
    CodexA12ModelBackendCompatibilityTest PRIVATE cxx_std_20
)
set_tests_properties(
    CodexA12ModelBackendCompatibilityTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;model;backend;reducer;compatibility;preservation;a1-2;b3"
        SKIP_RETURN_CODE 77
        TIMEOUT 15
)

snodec_add_test(
    CodexA12ConfigurationReadBackendPreservationTest
    CodexA12ConfigurationReadBackendPreservationTest.cpp
)
target_link_libraries(
    CodexA12ConfigurationReadBackendPreservationTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-backend
)
target_compile_features(
    CodexA12ConfigurationReadBackendPreservationTest PRIVATE cxx_std_20
)
set_tests_properties(
    CodexA12ConfigurationReadBackendPreservationTest
    PROPERTIES
        LABELS "component;ai;openai;codex;typed;configuration;backend;reducer;preservation;a1-2;b4"
        SKIP_RETURN_CODE 77
        TIMEOUT 10
)

snodec_add_test(CodexFrontendProtocolV1Test CodexFrontendProtocolV1Test.cpp)
target_link_libraries(
    CodexFrontendProtocolV1Test PRIVATE snodec-test-support
                                        snodec::ai-openai-codex-frontend
)
target_compile_features(CodexFrontendProtocolV1Test PRIVATE cxx_std_20)
set_tests_properties(
    CodexFrontendProtocolV1Test
    PROPERTIES LABELS
               "component;ai;openai;codex;frontend;protocol;journal;batch"
               SKIP_RETURN_CODE 77 TIMEOUT 10
)

snodec_add_test(CodexFrontendAdapterTest CodexFrontendAdapterTest.cpp)
target_link_libraries(
    CodexFrontendAdapterTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-frontend snodec::core
)
target_compile_features(CodexFrontendAdapterTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexFrontendAdapterTest
    PROPERTIES LABELS
               "component;ai;openai;codex;frontend;adapter;coalescing;session"
               SKIP_RETURN_CODE 77 TIMEOUT 15
)

snodec_add_test(
    CodexFrontendJsonLineFramerTest CodexFrontendJsonLineFramerTest.cpp
    ${PROJECT_SOURCE_DIR}/src/apps/codex-backend/JsonLineFramer.cpp
)
target_link_libraries(
    CodexFrontendJsonLineFramerTest PRIVATE snodec-test-support
)
target_include_directories(
    CodexFrontendJsonLineFramerTest PRIVATE ${PROJECT_SOURCE_DIR}/src
)
target_compile_features(CodexFrontendJsonLineFramerTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexFrontendJsonLineFramerTest
    PROPERTIES LABELS "component;ai;openai;codex;frontend;unix;framing"
               SKIP_RETURN_CODE 77 TIMEOUT 5
)

if(TARGET codex-backend-client-support)
    snodec_add_test(
        CodexBackendClientCommandTest CodexBackendClientCommandTest.cpp
    )
    target_link_libraries(
        CodexBackendClientCommandTest PRIVATE snodec-test-support
                                              codex-backend-client-support
    )
    target_compile_features(CodexBackendClientCommandTest PRIVATE cxx_std_20)
    set_tests_properties(
        CodexBackendClientCommandTest
        PROPERTIES LABELS
                   "component;ai;openai;codex;frontend;client;command;codec"
                   SKIP_RETURN_CODE 77 TIMEOUT 5
    )

    snodec_add_test(
        CodexBackendClientCommandDrainTest
        CodexBackendClientCommandDrainTest.cpp
    )
    target_link_libraries(
        CodexBackendClientCommandDrainTest PRIVATE snodec-test-support
                                                   codex-backend-client-support
    )
    target_compile_features(
        CodexBackendClientCommandDrainTest PRIVATE cxx_std_20
    )
    set_tests_properties(
        CodexBackendClientCommandDrainTest
        PROPERTIES LABELS
                   "component;ai;openai;codex;frontend;client;command;lifecycle"
                   SKIP_RETURN_CODE 77 TIMEOUT 5
    )

    snodec_add_test(
        CodexBackendClientPresenterTest CodexBackendClientPresenterTest.cpp
    )
    target_link_libraries(
        CodexBackendClientPresenterTest PRIVATE snodec-test-support
                                                codex-backend-client-support
    )
    target_compile_features(CodexBackendClientPresenterTest PRIVATE cxx_std_20)
    set_tests_properties(
        CodexBackendClientPresenterTest
        PROPERTIES LABELS
                   "component;ai;openai;codex;frontend;client;presentation"
                   SKIP_RETURN_CODE 77 TIMEOUT 5
    )

    snodec_add_test(
        CodexBackendClientJsonLineFramerTest
        CodexBackendClientJsonLineFramerTest.cpp
    )
    target_link_libraries(
        CodexBackendClientJsonLineFramerTest
        PRIVATE snodec-test-support codex-backend-client-support
    )
    target_compile_features(
        CodexBackendClientJsonLineFramerTest PRIVATE cxx_std_20
    )
    set_tests_properties(
        CodexBackendClientJsonLineFramerTest
        PROPERTIES LABELS
                   "component;ai;openai;codex;frontend;client;unix;framing"
                   SKIP_RETURN_CODE 77 TIMEOUT 5
    )

    snodec_add_test(
        CodexBackendClientStdinReaderTest CodexBackendClientStdinReaderTest.cpp
    )
    target_link_libraries(
        CodexBackendClientStdinReaderTest
        PRIVATE snodec-test-support codex-backend-client-support snodec::core
    )
    target_compile_features(
        CodexBackendClientStdinReaderTest PRIVATE cxx_std_20
    )
    set_tests_properties(
        CodexBackendClientStdinReaderTest
        PROPERTIES LABELS
                   "component;ai;openai;codex;frontend;client;stdin;lifecycle"
                   SKIP_RETURN_CODE 77 TIMEOUT 5
    )

    snodec_add_test(
        CodexBackendClientUnixAcceptanceTest
        CodexBackendClientUnixAcceptanceTest.cpp
    )
    target_link_libraries(
        CodexBackendClientUnixAcceptanceTest
        PRIVATE snodec-test-support codex-backend-client-support
                snodec::net-un-stream-legacy snodec::core
    )
    target_compile_features(
        CodexBackendClientUnixAcceptanceTest PRIVATE cxx_std_20
    )
    set_tests_properties(
        CodexBackendClientUnixAcceptanceTest
        PROPERTIES LABELS
                   "component;ai;openai;codex;frontend;client;unix;acceptance"
                   SKIP_RETURN_CODE 77 TIMEOUT 10
    )

    if(TARGET codex-backend-unix-adapter)
        snodec_add_test(
            CodexBackendClientRealBackendAcceptanceTest
            CodexBackendClientRealBackendAcceptanceTest.cpp
        )
        target_link_libraries(
            CodexBackendClientRealBackendAcceptanceTest
            PRIVATE snodec-test-support codex-backend-client-support
                    codex-backend-unix-adapter snodec::ai-openai-codex-backend
                    snodec::net-un-stream-legacy snodec::core
        )
        target_compile_features(
            CodexBackendClientRealBackendAcceptanceTest PRIVATE cxx_std_20
        )
        set_tests_properties(
            CodexBackendClientRealBackendAcceptanceTest
            PROPERTIES
                LABELS
                "component;ai;openai;codex;frontend;backend;client;unix;acceptance;interactive"
                ENVIRONMENT
                "SNODEC_CODEX_CLIENT_ACCEPTANCE_SCENARIO=interactive"
                SKIP_RETURN_CODE
                77
                TIMEOUT
                20
        )
        add_test(
            NAME CodexBackendClientThreadWorkflowAcceptanceTest
            COMMAND CodexBackendClientRealBackendAcceptanceTest
        )
        set_tests_properties(
            CodexBackendClientThreadWorkflowAcceptanceTest
            PROPERTIES
                LABELS
                "component;ai;openai;codex;frontend;backend;client;unix;acceptance;thread-workflow"
                ENVIRONMENT
                "SNODEC_CODEX_CLIENT_ACCEPTANCE_SCENARIO=thread-workflow"
                SKIP_RETURN_CODE
                77
                TIMEOUT
                20
        )
    endif()
endif()

snodec_add_test(CodexBackendSessionTest CodexBackendSessionTest.cpp)
target_link_libraries(
    CodexBackendSessionTest PRIVATE snodec-test-support
                                    snodec::ai-openai-codex-backend
)
target_compile_features(CodexBackendSessionTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexBackendSessionTest
    PROPERTIES LABELS "component;ai;openai;codex;backend;session"
               SKIP_RETURN_CODE 77 TIMEOUT 10
)

snodec_add_test(CodexBackendCoreTest CodexBackendCoreTest.cpp)
target_link_libraries(
    CodexBackendCoreTest PRIVATE snodec-test-support
                                 snodec::ai-openai-codex-backend snodec::core
)
target_compile_features(CodexBackendCoreTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexBackendCoreTest
    PROPERTIES LABELS "component;ai;openai;codex;backend;core" SKIP_RETURN_CODE
               77 TIMEOUT 15
)

snodec_add_test(
    CodexBackendUnixAcceptanceTest
    CodexBackendUnixAcceptanceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/apps/codex-backend/CodexFrontendSocketContext.cpp
    ${PROJECT_SOURCE_DIR}/src/apps/codex-backend/CodexFrontendSocketContextFactory.cpp
    ${PROJECT_SOURCE_DIR}/src/apps/codex-backend/JsonLineFramer.cpp
)
target_include_directories(
    CodexBackendUnixAcceptanceTest PRIVATE ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(
    CodexBackendUnixAcceptanceTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-frontend
            snodec::ai-openai-codex-backend snodec::net-un-stream-legacy
            snodec::core
)
target_compile_features(CodexBackendUnixAcceptanceTest PRIVATE cxx_std_20)
set_tests_properties(
    CodexBackendUnixAcceptanceTest
    PROPERTIES LABELS
               "component;ai;openai;codex;backend;frontend;unix;acceptance"
               SKIP_RETURN_CODE 77 TIMEOUT 20
)

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

snodec_add_test(CodexTypedClientFacadeTest CodexTypedClientFacadeTest.cpp)
target_link_libraries(
    CodexTypedClientFacadeTest PRIVATE snodec-test-support
                                       snodec::ai-openai-codex
)
target_compile_features(CodexTypedClientFacadeTest PRIVATE cxx_std_20)
target_compile_options(
    CodexTypedClientFacadeTest PRIVATE -Wno-deprecated-declarations
)
set_tests_properties(
    CodexTypedClientFacadeTest
    PROPERTIES LABELS "component;ai;openai;codex;typed;facade;compatibility"
               TIMEOUT 5
)

add_test(
    NAME CodexTypedFacadeUsageGuardTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexTypedFacadeUsageGuardTest.py
        --source-root ${PROJECT_SOURCE_DIR}/src
)
set_tests_properties(
    CodexTypedFacadeUsageGuardTest
    PROPERTIES LABELS "component;ai;openai;codex;typed;facade;policy"
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

snodec_add_test(CodexBackendLifecycleTest CodexBackendLifecycleTest.cpp)
target_include_directories(
    CodexBackendLifecycleTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_compile_features(CodexBackendLifecycleTest PRIVATE cxx_std_20)
target_link_libraries(
    CodexBackendLifecycleTest
    PRIVATE snodec-test-support snodec::ai-openai-codex-backend snodec::logger
)
set_tests_properties(
    CodexBackendLifecycleTest
    PROPERTIES LABELS "component;codex;backend;thread;turn;lifecycle;log"
               SKIP_RETURN_CODE 77 TIMEOUT 10
)

snodec_add_test(
    AppServerClientRequestLifecycleTest
    AppServerClientRequestLifecycleTest.cpp
)
target_include_directories(
    AppServerClientRequestLifecycleTest PRIVATE ${PROJECT_SOURCE_DIR}
)
target_compile_features(
    AppServerClientRequestLifecycleTest PRIVATE cxx_std_20
)
target_link_libraries(
    AppServerClientRequestLifecycleTest
    PRIVATE snodec-test-support snodec::ai-openai-codex snodec::core
            snodec::logger
)
set_tests_properties(
    AppServerClientRequestLifecycleTest
    PROPERTIES LABELS "component;codex;protocol;request;lifecycle;log"
               SKIP_RETURN_CODE 77 TIMEOUT 10
)
