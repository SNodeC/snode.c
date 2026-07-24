if(NOT DEFINED SNODEC_BUILD_DIR OR NOT DEFINED SNODEC_SOURCE_DIR
   OR NOT DEFINED CMAKE_CPACK_COMMAND)
    message(
        FATAL_ERROR
            "SNODEC_BUILD_DIR, SNODEC_SOURCE_DIR, and CMAKE_CPACK_COMMAND are required"
    )
endif()

set(source_package_stage
    "${SNODEC_BUILD_DIR}/codex-source-package-audit"
)
file(REMOVE_RECURSE "${source_package_stage}")
file(MAKE_DIRECTORY "${source_package_stage}")

execute_process(
    COMMAND
        "${CMAKE_CPACK_COMMAND}" -G TGZ --config
        "${SNODEC_BUILD_DIR}/CPackSourceConfig.cmake" -D
        "CPACK_OUTPUT_FILE_PREFIX=${source_package_stage}"
    WORKING_DIRECTORY "${SNODEC_BUILD_DIR}"
    RESULT_VARIABLE package_result
    OUTPUT_VARIABLE package_output
    ERROR_VARIABLE package_error
)
if(NOT package_result EQUAL 0)
    message(
        FATAL_ERROR
            "Codex source-package generation failed\n${package_output}\n${package_error}"
    )
endif()

file(GLOB source_archives "${source_package_stage}/*.tar.gz")
list(LENGTH source_archives source_archive_count)
if(NOT source_archive_count EQUAL 1)
    message(
        FATAL_ERROR
            "expected exactly one TGZ source package, found ${source_archive_count}: ${source_archives}"
    )
endif()
list(GET source_archives 0 source_archive)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar tf "${source_archive}"
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE archive_entries
    ERROR_VARIABLE list_error
)
if(NOT list_result EQUAL 0)
    message(
        FATAL_ERROR
            "could not list Codex source package ${source_archive}\n${list_error}"
    )
endif()
string(REPLACE "\r\n" "\n" archive_entries "${archive_entries}")

string(REPLACE "\n" ";" archive_entry_lines "${archive_entries}")
set(archive_source_prefixes)
foreach(archive_entry IN LISTS archive_entry_lines)
    if(archive_entry MATCHES "^(.*)/\\.gitattributes$")
        list(APPEND archive_source_prefixes "${CMAKE_MATCH_1}/")
    endif()
endforeach()
list(REMOVE_DUPLICATES archive_source_prefixes)
list(LENGTH archive_source_prefixes archive_source_prefix_count)
if(NOT archive_source_prefix_count EQUAL 1)
    message(
        FATAL_ERROR
            "expected one packaged source root identified by .gitattributes, found ${archive_source_prefix_count}: ${archive_source_prefixes}"
    )
endif()
list(GET archive_source_prefixes 0 archive_source_prefix)
string(LENGTH "${archive_source_prefix}" archive_source_prefix_length)

set(archive_relative_files)
foreach(archive_entry IN LISTS archive_entry_lines)
    if(archive_entry STREQUAL "" OR archive_entry MATCHES "/$")
        continue()
    endif()
    string(FIND "${archive_entry}" "${archive_source_prefix}" source_position)
    if(source_position EQUAL 0)
        string(
            SUBSTRING "${archive_entry}" ${archive_source_prefix_length} -1
                      relative_entry
        )
        list(APPEND archive_relative_files "${relative_entry}")
    endif()
endforeach()
list(SORT archive_relative_files)

function(assert_retained_prefix relative_prefix pinned_file_count)
    file(
        GLOB_RECURSE expected_files
        LIST_DIRECTORIES FALSE
        RELATIVE "${SNODEC_SOURCE_DIR}"
        "${SNODEC_SOURCE_DIR}/${relative_prefix}/*"
    )
    list(SORT expected_files)
    list(LENGTH expected_files expected_file_count)
    if(NOT expected_file_count EQUAL pinned_file_count)
        message(
            FATAL_ERROR
                "pinned ${relative_prefix} source count changed: expected ${pinned_file_count}, found ${expected_file_count}"
        )
    endif()

    set(observed_files)
    foreach(archive_file IN LISTS archive_relative_files)
        string(FIND "${archive_file}" "${relative_prefix}/" prefix_position)
        if(prefix_position EQUAL 0)
            list(APPEND observed_files "${archive_file}")
        endif()
    endforeach()
    list(SORT observed_files)
    list(LENGTH observed_files observed_file_count)

    if(NOT observed_file_count EQUAL expected_file_count)
        message(
            FATAL_ERROR
                "source-package ${relative_prefix} count mismatch: expected ${expected_file_count}, observed ${observed_file_count}"
        )
    endif()

    foreach(expected_file IN LISTS expected_files)
        list(FIND observed_files "${expected_file}" observed_position)
        if(observed_position EQUAL -1)
            message(
                FATAL_ERROR
                    "source-package omitted ${relative_prefix} file: ${expected_file}"
            )
        endif()
    endforeach()
    foreach(observed_file IN LISTS observed_files)
        list(FIND expected_files "${observed_file}" expected_position)
        if(expected_position EQUAL -1)
            message(
                FATAL_ERROR
                    "source-package contains unexpected ${relative_prefix} file: ${observed_file}"
            )
        endif()
    endforeach()

    message(
        STATUS
            "source-package retained ${relative_prefix}: expected=${expected_file_count}, observed=${observed_file_count}"
    )
endfunction()

# These exact pinned counts make omission of any authoritative input or fixture
# mechanically visible. The fixture generator's extracted-package check below
# additionally proves that all 4320 index records resolve to the retained files
# with their recorded hashes and that no stale or extra fixture exists.
assert_retained_prefix("tools/codex/app-server-schema/0.144.6" 607)
assert_retained_prefix("tools/codex/app-server-fixtures/0.144.6" 4321)
assert_retained_prefix(
    "tools/codex/app-server-protocol-source/0.144.6" 4
)
assert_retained_prefix("tools/codex/app-server-evidence/0.144.6" 16)
assert_retained_prefix("tools/codex/app-server-surface" 1)
assert_retained_prefix("docs/ai/openai/codex" 15)
assert_retained_prefix(
    "tests/component/codex/fixtures/app-server-0.144.6" 7
)

file(
    GLOB top_level_codex_tools
    LIST_DIRECTORIES FALSE
    RELATIVE "${SNODEC_SOURCE_DIR}"
    "${SNODEC_SOURCE_DIR}/tools/codex/*"
)
list(SORT top_level_codex_tools)
list(LENGTH top_level_codex_tools top_level_codex_tool_count)
if(NOT top_level_codex_tool_count EQUAL 10)
    message(
        FATAL_ERROR
            "pinned top-level Codex tool count changed: expected 10, found ${top_level_codex_tool_count}"
    )
endif()
set(observed_top_level_codex_tools)
foreach(archive_file IN LISTS archive_relative_files)
    if(archive_file MATCHES "^tools/codex/[^/]+$")
        list(APPEND observed_top_level_codex_tools "${archive_file}")
    endif()
endforeach()
list(SORT observed_top_level_codex_tools)
list(LENGTH observed_top_level_codex_tools observed_top_level_tool_count)
if(NOT "${top_level_codex_tools}" STREQUAL
   "${observed_top_level_codex_tools}")
    message(
        FATAL_ERROR
            "source-package top-level Codex tools differ: expected ${top_level_codex_tools}; observed ${observed_top_level_codex_tools}"
    )
endif()
message(
    STATUS
        "source-package retained top-level Codex tools: expected=${top_level_codex_tool_count}, observed=${observed_top_level_tool_count}"
)

set(
    required_source_entries
    ".gitattributes"
    "tools/codex/app-server-schema/0.144.6/PROVENANCE.json"
    "tools/codex/app-server-schema/0.144.6/stable/ClientRequest.json"
    "tools/codex/app-server-schema/0.144.6/experimental/ClientRequest.json"
    "tools/codex/app-server-surface/0.144.6.json"
    "tools/codex/app-server-protocol-source/0.144.6/PROVENANCE.json"
    "tools/codex/app-server-protocol-source/0.144.6/LICENSE.openai-codex"
    "tools/codex/app-server-protocol-source/0.144.6/NOTICE.openai-codex"
    "tools/codex/app-server-protocol-source/0.144.6/codex-rs/app-server-protocol/src/protocol/common.rs"
    "tools/codex/app-server-evidence/0.144.6/operation-contracts.json"
    "tools/codex/app-server-evidence/0.144.6/typescript-audit.json"
    "tools/codex/app-server-evidence/0.144.6/module-slice-assignment.json"
    "tools/codex/app-server-evidence/0.144.6/nested-reachability.json"
    "tools/codex/app-server-evidence/0.144.6/schema-completeness-evidence.json"
    "tools/codex/app-server-evidence/0.144.6/fixture-coverage.json"
    "tools/codex/app-server-evidence/0.144.6/schema-keywords.json"
    "tools/codex/app-server-evidence/0.144.6/a1-1-start-state.json"
    "tools/codex/app-server-evidence/0.144.6/a1-1-implementation-plan.json"
    "tools/codex/app-server-evidence/0.144.6/a1-1-type-closure.json"
    "tools/codex/app-server-evidence/0.144.6/a1-1-operation-production-coverage.json"
    "tools/codex/app-server-evidence/0.144.6/a1-1-notification-production-coverage.json"
    "tools/codex/app-server-evidence/0.144.6/a1-1-closure-report.json"
    "tools/codex/app-server-evidence/0.144.6/a1-2-start-state.json"
    "tools/codex/app-server-evidence/0.144.6/a1-2-implementation-plan.json"
    "tools/codex/app-server-evidence/0.144.6/a1-2-type-closure.json"
    "tools/codex/app-server-fixtures/0.144.6/index.json"
    "tools/codex/app_server_a1_1.py"
    "tools/codex/app_server_a1_1_closure.py"
    "tools/codex/app_server_a1_2.py"
    "tools/codex/app_server_a1_shared.py"
    "tools/codex/app_server_schema_paths.py"
    "tools/codex/app_server_surface.py"
    "tools/codex/app_server_contracts.py"
    "tools/codex/app_server_fixtures.py"
    "tools/codex/draft07.py"
    "src/ai/openai/codex/detail/AccountCodec.cpp"
    "src/ai/openai/codex/detail/AccountCodec.h"
    "src/ai/openai/codex/detail/AccountsModelsConfigurationUnionCodecDescriptors.inc"
    "src/ai/openai/codex/detail/ConversationCodec.cpp"
    "src/ai/openai/codex/detail/ConversationCodec.h"
    "src/ai/openai/codex/detail/ConversationUnionCodecDescriptors.inc"
    "src/ai/openai/codex/detail/ClientOperationCodecDescriptors.inc"
    "src/ai/openai/codex/detail/ClientOperationCodec.cpp"
    "src/ai/openai/codex/detail/ClientOperationCodec.h"
    "src/ai/openai/codex/detail/ModelCodec.cpp"
    "src/ai/openai/codex/detail/ModelCodec.h"
    "src/ai/openai/codex/detail/ThreadItemCodecDescriptors.inc"
    "src/ai/openai/codex/detail/ResponseItemCodecDescriptors.inc"
    "src/ai/openai/codex/detail/ServerRequestCodecDescriptors.inc"
    "src/ai/openai/codex/detail/ServerNotificationCodecDescriptors.inc"
    "src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc"
    "src/ai/openai/codex/typed/Accounts.cpp"
    "src/ai/openai/codex/typed/Accounts.h"
    "src/ai/openai/codex/typed/Conversation.h"
    "src/ai/openai/codex/typed/Events.h"
    "src/ai/openai/codex/typed/Models.cpp"
    "src/ai/openai/codex/typed/Models.h"
    "tests/CodexBinaryPackageTest.cmake"
    "tests/component/codex/CodexA11B2TypedSurfaceBaseline.h"
    "tests/component/codex/CodexA11ArtifactByteIdentityTest.py"
    "tests/component/codex/CodexA11AuditToolTest.py"
    "tests/component/codex/CodexA12AuditToolTest.py"
    "tests/component/codex/CodexA12DocumentationConsistencyTest.py"
    "tests/component/codex/CodexAppServerContractsToolTest.py"
    "tests/component/codex/CodexAppServerFixtureToolTest.py"
    "tests/component/codex/CodexConversationCodecTest.cpp"
    "tests/component/codex/CodexA11OperationResultCorpusTest.cpp"
    "tests/component/codex/CodexA11OperationAggregateValueCorpusTest.cpp"
    "tests/component/codex/CodexConversationB4NestedCodecTest.cpp"
    "tests/component/codex/CodexA11OperationWireTest.cpp"
    "tests/component/codex/CodexA11NotificationCodecTest.cpp"
    "tests/component/codex/CodexA11NotificationBackendPreservationTest.cpp"
    "tests/component/codex/CodexA12AccountCodecTest.cpp"
    "tests/component/codex/CodexA12AccountWireTest.cpp"
    "tests/component/codex/CodexA12AccountBackendPreservationTest.cpp"
    "tests/component/codex/CodexA12AuthRefreshWireTest.cpp"
    "tests/component/codex/CodexA12ModelCodecTest.cpp"
    "tests/component/codex/CodexA12ModelWireTest.cpp"
    "tests/component/codex/CodexA12ModelBackendCompatibilityTest.cpp"
    "tests/component/codex/CodexDraft07ValidatorTest.py"
    "tests/component/codex/CodexTypedClientFacadeTest.cpp"
    "tests/component/codex/CodexTypedFacadeUsageGuardTest.py"
    "tests/installed/codex/CodexTypedConsumer.cpp"
    "docs/ai/openai/codex/a1-1-conversation-domain.md"
    "docs/ai/openai/codex/a1-1-test-integrity.md"
    "docs/ai/openai/codex/a1-2-accounts-models-configuration.md"
    "docs/ai/openai/codex/a1-typed-foundation.md"
)
foreach(required_entry IN LISTS required_source_entries)
    string(FIND "${archive_entries}" "/${required_entry}\n" entry_position)
    if(entry_position EQUAL -1)
        message(
            FATAL_ERROR
                "Codex reproducibility input missing from source package: ${required_entry}"
        )
    endif()
endforeach()

set(extracted_stage "${source_package_stage}/extracted")
file(MAKE_DIRECTORY "${extracted_stage}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xzf "${source_archive}"
    WORKING_DIRECTORY "${extracted_stage}"
    RESULT_VARIABLE extract_result
    ERROR_VARIABLE extract_error
)
if(NOT extract_result EQUAL 0)
    message(
        FATAL_ERROR
            "could not extract Codex source package ${source_archive}\n${extract_error}"
    )
endif()
file(
    GLOB_RECURSE extracted_root_markers
    LIST_DIRECTORIES FALSE
    "${extracted_stage}/*/.gitattributes"
)
list(LENGTH extracted_root_markers extracted_root_count)
if(NOT extracted_root_count EQUAL 1)
    message(
        FATAL_ERROR
            "expected one extracted source-package root marker, found ${extracted_root_count}: ${extracted_root_markers}"
    )
endif()
list(GET extracted_root_markers 0 extracted_root_marker)
get_filename_component(extracted_root "${extracted_root_marker}" DIRECTORY)
if(NOT IS_DIRECTORY "${extracted_root}")
    message(FATAL_ERROR "source-package root is not a directory: ${extracted_root}")
endif()

find_program(CODEX_PYTHON_EXECUTABLE NAMES python3)
if(NOT CODEX_PYTHON_EXECUTABLE)
    message(FATAL_ERROR "python3 is required for extracted source-package checks")
endif()

function(run_extracted_check check_name)
    execute_process(
        COMMAND ${ARGN}
        WORKING_DIRECTORY "${extracted_root}"
        RESULT_VARIABLE check_result
        OUTPUT_VARIABLE check_output
        ERROR_VARIABLE check_error
    )
    if(NOT check_result EQUAL 0)
        message(
            FATAL_ERROR
                "extracted source-package ${check_name} failed\n${check_output}\n${check_error}"
        )
    endif()
    message(STATUS "extracted source-package ${check_name}: passed")
endfunction()

set(extracted_schema_root
    "${extracted_root}/tools/codex/app-server-schema/0.144.6"
)
set(extracted_surface
    "${extracted_root}/tools/codex/app-server-surface/0.144.6.json"
)
set(extracted_source_root
    "${extracted_root}/tools/codex/app-server-protocol-source/0.144.6"
)
set(extracted_evidence_root
    "${extracted_root}/tools/codex/app-server-evidence/0.144.6"
)
set(extracted_fixture_root
    "${extracted_root}/tools/codex/app-server-fixtures/0.144.6"
)

# Run the guards from the extracted archive, not the checkout. This validates
# every schema/provenance hash, all provenance-listed Rust files, the exact
# operation contracts/reports, A1.2 audit regeneration, the full indexed
# fixture corpus, deterministic regeneration, and all required-field/wrong-type
# mutations using only packaged inputs.
run_extracted_check(
    "schema provenance/integrity"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_surface.py"
    verify
    --schema-root
    "${extracted_schema_root}"
    --provenance
    "${extracted_schema_root}/PROVENANCE.json"
    --manifest
    "${extracted_surface}"
)
run_extracted_check(
    "Rust/schema operation contracts"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_contracts.py"
    --source-root
    "${extracted_source_root}"
    --schema-root
    "${extracted_schema_root}"
    --manifest
    "${extracted_surface}"
    --schema-provenance
    "${extracted_schema_root}/PROVENANCE.json"
    --evidence-root
    "${extracted_evidence_root}"
    --check
)
run_extracted_check(
    "A1.1 operation descriptor generation"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_surface.py"
    operation-descriptors
    --manifest
    "${extracted_surface}"
    --evidence-root
    "${extracted_evidence_root}"
    --output
    "${extracted_root}/src/ai/openai/codex/detail/ClientOperationCodecDescriptors.inc"
    --check
)
run_extracted_check(
    "A1.1 operation production coverage"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_surface.py"
    operation-production-coverage
    --manifest
    "${extracted_surface}"
    --evidence-root
    "${extracted_evidence_root}"
    --fixture-index
    "${extracted_fixture_root}/index.json"
    --repo-root
    "${extracted_root}"
    --output
    "${extracted_evidence_root}/a1-1-operation-production-coverage.json"
    --check
)
run_extracted_check(
    "A1.1 notification descriptor generation"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_surface.py"
    notification-descriptors
    --manifest
    "${extracted_surface}"
    --evidence-root
    "${extracted_evidence_root}"
    --output
    "${extracted_root}/src/ai/openai/codex/detail/ServerNotificationCodecDescriptors.inc"
    --check
)
run_extracted_check(
    "A1.1 notification production coverage"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_surface.py"
    notification-production-coverage
    --manifest
    "${extracted_surface}"
    --evidence-root
    "${extracted_evidence_root}"
    --fixture-index
    "${extracted_fixture_root}/index.json"
    --repo-root
    "${extracted_root}"
    --output
    "${extracted_evidence_root}/a1-1-notification-production-coverage.json"
    --check
)
run_extracted_check(
    "A1.1 shared conversation descriptor generation"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_surface.py"
    conversation-descriptors
    --manifest
    "${extracted_surface}"
    --schema-root
    "${extracted_schema_root}"
    --evidence-root
    "${extracted_evidence_root}"
    --output
    "${extracted_root}/src/ai/openai/codex/detail/ConversationUnionCodecDescriptors.inc"
    --check
)
run_extracted_check(
    "A1.1 item descriptor generation"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_surface.py"
    item-descriptors
    --manifest
    "${extracted_surface}"
    --schema-root
    "${extracted_schema_root}"
    --evidence-root
    "${extracted_evidence_root}"
    --thread-output
    "${extracted_root}/src/ai/openai/codex/detail/ThreadItemCodecDescriptors.inc"
    --response-output
    "${extracted_root}/src/ai/openai/codex/detail/ResponseItemCodecDescriptors.inc"
    --check
)
run_extracted_check(
    "A1.1 implementation-plan/type-closure audit"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_a1_1.py"
    check
    --repo-root
    "${extracted_root}"
    --start-state
    "${extracted_evidence_root}/a1-1-start-state.json"
    --plan-output
    "${extracted_evidence_root}/a1-1-implementation-plan.json"
    --closure-output
    "${extracted_evidence_root}/a1-1-type-closure.json"
)
run_extracted_check(
    "A1.2 implementation-plan/type-closure audit"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_a1_2.py"
    check
    --repo-root
    "${extracted_root}"
    --start-state
    "${extracted_evidence_root}/a1-2-start-state.json"
    --plan-output
    "${extracted_evidence_root}/a1-2-implementation-plan.json"
    --closure-output
    "${extracted_evidence_root}/a1-2-type-closure.json"
)
run_extracted_check(
    "fixture deterministic check"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_fixtures.py"
    check
    --repo-root
    "${extracted_root}"
    --schema-root
    "${extracted_schema_root}"
    --manifest
    "${extracted_surface}"
    --contracts
    "${extracted_evidence_root}/operation-contracts.json"
    --fixture-root
    "${extracted_fixture_root}"
    --evidence-root
    "${extracted_evidence_root}"
)
run_extracted_check(
    "fixture validation/mutations"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_fixtures.py"
    validate
    --repo-root
    "${extracted_root}"
    --schema-root
    "${extracted_schema_root}"
    --manifest
    "${extracted_surface}"
    --contracts
    "${extracted_evidence_root}/operation-contracts.json"
    --fixture-root
    "${extracted_fixture_root}"
    --evidence-root
    "${extracted_evidence_root}"
)
run_extracted_check(
    "A1.1 final closure report"
    "${CODEX_PYTHON_EXECUTABLE}"
    "${extracted_root}/tools/codex/app_server_a1_1_closure.py"
    check
    --repo-root
    "${extracted_root}"
)

set(
    forbidden_source_patterns
    "/.git/"
    "/.agents/"
    "/.codex/"
    "/.kdev4/"
    "/_CPack_Packages/"
    "/__pycache__/"
    ".kdev4"
    ".pyc"
    "/build/"
    "/build-a1-0a/"
    "/build-a1-0/"
)
foreach(forbidden_pattern IN LISTS forbidden_source_patterns)
    string(FIND "${archive_entries}" "${forbidden_pattern}" entry_position)
    if(NOT entry_position EQUAL -1)
        message(
            FATAL_ERROR
                "local-only artifact leaked into source package: ${forbidden_pattern}"
        )
    endif()
endforeach()
