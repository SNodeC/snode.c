find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

set(AISUITE_CUTOVER_TOOL
    "${PROJECT_SOURCE_DIR}/tools/ownership/verify_aisuite_cutover.py"
)
set(AISUITE_CUTOVER_START_EVIDENCE
    "${PROJECT_SOURCE_DIR}/docs/migrations/aisuite-cutover-start-state.json"
)

add_test(NAME AISuiteCutoverStartStateAuditTest
         COMMAND ${Python3_EXECUTABLE} ${AISUITE_CUTOVER_TOOL} audit-start
                 --repo-root ${PROJECT_SOURCE_DIR}
)
set_tests_properties(
    AISuiteCutoverStartStateAuditTest
    PROPERTIES LABELS "policy;architecture;ownership;cutover;package"
               ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 120
)

add_test(
    NAME AISuiteCutoverStartStateMutationTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/AISuiteCutoverStartStateMutationTest.py
        --tool ${AISUITE_CUTOVER_TOOL} --evidence
        ${AISUITE_CUTOVER_START_EVIDENCE}
)
set_tests_properties(
    AISuiteCutoverStartStateMutationTest
    PROPERTIES LABELS "policy;architecture;ownership;cutover;package"
               ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 30
)

add_test(
    NAME AISuiteOwnershipBoundaryPolicyTest
    COMMAND ${Python3_EXECUTABLE} ${AISUITE_CUTOVER_TOOL} check --repo-root
            ${PROJECT_SOURCE_DIR} --build-root ${CMAKE_BINARY_DIR}
)
set_tests_properties(
    AISuiteOwnershipBoundaryPolicyTest
    PROPERTIES LABELS "policy;architecture;ownership;cutover;package"
               ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 120
)

add_test(
    NAME AISuiteCutoverMutationTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/AISuiteCutoverMutationTest.py --tool
        ${AISUITE_CUTOVER_TOOL} --evidence
        ${PROJECT_SOURCE_DIR}/docs/migrations/aisuite-cutover-boundary.json
)
set_tests_properties(
    AISuiteCutoverMutationTest
    PROPERTIES LABELS "policy;architecture;ownership;cutover;package"
               ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 60
)

set(AISUITE_CUTOVER_CLOSURE_TOOL
    "${PROJECT_SOURCE_DIR}/tools/ownership/verify_aisuite_cutover_closure.py"
)

add_test(
    NAME AISuiteCutoverHistoryTopologyTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/AISuiteCutoverClosureTest.py --tool
        ${AISUITE_CUTOVER_CLOSURE_TOOL} --suite topology
)
set_tests_properties(
    AISuiteCutoverHistoryTopologyTest
    PROPERTIES LABELS "policy;architecture;ownership;cutover;package"
               ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1" TIMEOUT 120
)

add_test(
    NAME AISuiteCutoverClosureTest
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/AISuiteCutoverClosureTest.py --tool
        ${AISUITE_CUTOVER_CLOSURE_TOOL} --suite closure --repo-root
        ${PROJECT_SOURCE_DIR} --build-root ${CMAKE_BINARY_DIR}
)
set_tests_properties(
    AISuiteCutoverClosureTest
    PROPERTIES
        LABELS
        "policy;architecture;ownership;cutover;package"
        ENVIRONMENT
        "PYTHONDONTWRITEBYTECODE=1"
        TIMEOUT
        180
        DEPENDS
        "AISuiteCutoverHistoryTopologyTest;AISuiteOwnershipBoundaryPolicyTest;AISuiteCutoverMutationTest;StagedInstalledConsumerTest;AISuiteCutoverSourcePackageTest;AISuiteCutoverBinaryPackageTest"
)
