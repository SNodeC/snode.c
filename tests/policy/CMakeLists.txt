function(snodec_add_policy_test test_name source labels)
    snodec_add_test(${test_name} ${source})
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR})
    target_compile_features(${test_name} PRIVATE cxx_std_20)
    set_tests_properties(
        ${test_name}
        PROPERTIES LABELS "policy;${labels}"
                   ENVIRONMENT "SNODEC_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
                   TIMEOUT 10
    )
endfunction()

snodec_add_policy_test(
    ParameterlessSemanticLoggerPolicyTest
    log/ParameterlessSemanticLoggerPolicyTest.cpp
    "log;architecture"
)
snodec_add_policy_test(
    LoggingApiSurfacePolicyTest
    log/LoggingApiSurfacePolicyTest.cpp
    "log;architecture;api"
)
snodec_add_policy_test(
    EpollDescriptorPublisherPolicyTest
    core/EpollDescriptorPublisherPolicyTest.cpp
    "core;epoll"
)
snodec_add_policy_test(
    EventLoopSyscallDisciplinePolicyTest
    core/EventLoopSyscallDisciplinePolicyTest.cpp
    "core;syscall"
)
snodec_add_policy_test(
    SensitiveLoggingPolicyTest
    security/SensitiveLoggingPolicyTest.cpp
    "security;log"
)
snodec_add_policy_test(
    CiWorkflowPathsPolicyTest ci/CiWorkflowPathsPolicyTest.cpp
    "ci;codex;phase-a0;phase-a1-0"
)
snodec_add_policy_test(
    CodexA12PublicHeaderPolicyTest
    codex/CodexA12PublicHeaderPolicyTest.cpp
    "codex;phase-a1-2;public-api;headers"
)

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

add_test(
    NAME CodexSyntheticSecretLeakGuardTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/security/CodexSyntheticSecretLeakGuardTest.py
        --repo-root
        ${PROJECT_SOURCE_DIR}
        --build-root
        ${CMAKE_BINARY_DIR}
)
set(
    CODEX_SYNTHETIC_SECRET_LEAK_GUARD_DEPENDENCIES
    StagedInstalledConsumerTest
    CodexSourcePackageTest
    CodexBinaryPackageTest
    CodexAppServerGeneratedArtifactsGuardTest
    CodexA12DocumentationConsistencyTest
    CodexA12ClosureEvidenceTest
    CodexAppServerFixtureInfrastructureTest
    CodexA12PublicHeaderPolicyTest
)
set_tests_properties(
    CodexSyntheticSecretLeakGuardTest
    PROPERTIES
        LABELS "policy;security;codex;phase-a1-2;generated;package"
        ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1"
        DEPENDS
            "${CODEX_SYNTHETIC_SECRET_LEAK_GUARD_DEPENDENCIES}"
        RUN_SERIAL TRUE
        TIMEOUT 120
)
