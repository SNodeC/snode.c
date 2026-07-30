if(NOT DEFINED SNODEC_BUILD_DIR
   OR NOT DEFINED SNODEC_SOURCE_DIR
   OR NOT DEFINED CMAKE_CPACK_COMMAND
)
    message(
        FATAL_ERROR
            "SNODEC_BUILD_DIR, SNODEC_SOURCE_DIR, and CMAKE_CPACK_COMMAND are required"
    )
endif()

find_program(AISUITE_CUTOVER_PYTHON NAMES python3 REQUIRED)

if(DEFINED ENV{RUNNER_TEMP} AND NOT "$ENV{RUNNER_TEMP}" STREQUAL "")
    set(external_temp_root "$ENV{RUNNER_TEMP}")
elseif(DEFINED ENV{TMPDIR} AND NOT "$ENV{TMPDIR}" STREQUAL "")
    set(external_temp_root "$ENV{TMPDIR}")
else()
    set(external_temp_root "/tmp")
endif()
get_filename_component(external_temp_root "${external_temp_root}" ABSOLUTE)
string(SHA256 source_package_stage_key
              "${SNODEC_SOURCE_DIR}|${SNODEC_BUILD_DIR}"
)
string(SUBSTRING "${source_package_stage_key}" 0 16 source_package_stage_key)
set(source_package_stage
    "${external_temp_root}/snodec-aisuite-cutover-source-package-${source_package_stage_key}"
)
set(extracted_stage "${source_package_stage}/extracted")
set(empty_home "${source_package_stage}/empty-home")
set(source_package_manifest "${source_package_stage}/source-manifest.txt")

get_filename_component(source_root_absolute "${SNODEC_SOURCE_DIR}" ABSOLUTE)
get_filename_component(stage_root_absolute "${source_package_stage}" ABSOLUTE)
string(FIND "${stage_root_absolute}/" "${source_root_absolute}/"
            stage_below_source
)
if(stage_below_source EQUAL 0)
    message(
        FATAL_ERROR
            "package-safe stage must be outside the live source checkout: ${stage_root_absolute}"
    )
endif()

file(REMOVE_RECURSE "${source_package_stage}")
file(MAKE_DIRECTORY "${source_package_stage}" "${extracted_stage}"
     "${empty_home}"
)

execute_process(
    COMMAND
        "${CMAKE_CPACK_COMMAND}" -G TGZ --config
        "${SNODEC_BUILD_DIR}/CPackSourceConfig.cmake" -D
        "CPACK_OUTPUT_FILE_PREFIX=${source_package_stage}" -D
        "CPACK_PACKAGING_INSTALL_PREFIX=/"
    WORKING_DIRECTORY "${SNODEC_BUILD_DIR}"
    RESULT_VARIABLE package_result
    OUTPUT_VARIABLE package_output
    ERROR_VARIABLE package_error
)
if(NOT package_result EQUAL 0)
    message(
        FATAL_ERROR
            "AISuite cutover source-package generation failed\n${package_output}\n${package_error}"
    )
endif()

file(
    GLOB source_archives
    LIST_DIRECTORIES FALSE
    "${source_package_stage}/*.tar.gz"
)
list(SORT source_archives)
list(LENGTH source_archives source_archive_count)
if(NOT source_archive_count EQUAL 1)
    message(
        FATAL_ERROR
            "expected exactly one fresh TGZ source package, found ${source_archive_count}: ${source_archives}"
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
            "could not list AISuite cutover source package ${source_archive}\n${list_error}"
    )
endif()
string(REPLACE "\r\n" "\n" archive_entries "${archive_entries}")
string(REPLACE "\n" ";" archive_entry_lines "${archive_entries}")
foreach(archive_entry IN LISTS archive_entry_lines)
    if(archive_entry MATCHES "(^|/)\\.\\.(/|$)")
        message(
            FATAL_ERROR
                "unsafe parent traversal in source package: ${archive_entry}"
        )
    endif()
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xzf "${source_archive}"
    WORKING_DIRECTORY "${extracted_stage}"
    RESULT_VARIABLE extract_result
    OUTPUT_VARIABLE extract_output
    ERROR_VARIABLE extract_error
)
if(NOT extract_result EQUAL 0)
    message(
        FATAL_ERROR
            "could not extract AISuite cutover source package ${source_archive}\n${extract_output}\n${extract_error}"
    )
endif()

file(
    GLOB_RECURSE cmake_candidates
    LIST_DIRECTORIES FALSE
    "${extracted_stage}/*/CMakeLists.txt"
)
set(source_roots)
foreach(cmake_candidate IN LISTS cmake_candidates)
    file(READ "${cmake_candidate}" cmake_candidate_content)
    if(cmake_candidate_content MATCHES "project\\([\r\n\t ]*SNode\\.C[\r\n\t ]")
        get_filename_component(source_root "${cmake_candidate}" DIRECTORY)
        list(APPEND source_roots "${source_root}")
    endif()
endforeach()
list(REMOVE_DUPLICATES source_roots)
list(LENGTH source_roots source_root_count)
if(NOT source_root_count EQUAL 1)
    message(
        FATAL_ERROR
            "expected one extracted SNode.C source root, found ${source_root_count}: ${source_roots}"
    )
endif()
list(GET source_roots 0 source_root)

foreach(root_file IN ITEMS CMakeLists.txt LICENSE README.md)
    if(NOT EXISTS "${source_root}/${root_file}")
        message(
            FATAL_ERROR
                "extracted SNode.C source root lacks ${root_file}: ${source_root}"
        )
    endif()
endforeach()

file(
    GLOB_RECURSE source_package_files
    LIST_DIRECTORIES FALSE
    RELATIVE "${source_root}"
    "${source_root}/*"
)
list(SORT source_package_files)
set(normalized_source_package_files)
foreach(source_package_file IN LISTS source_package_files)
    string(REPLACE "\\" "/" source_package_file "${source_package_file}")
    if(source_package_file MATCHES "^\\./")
        string(SUBSTRING "${source_package_file}" 2 -1 source_package_file)
    endif()
    list(APPEND normalized_source_package_files "${source_package_file}")
endforeach()
set(source_package_files "${normalized_source_package_files}")
list(REMOVE_DUPLICATES source_package_files)
string(REPLACE ";" "\n" source_package_manifest_content
               "${source_package_files}"
)
file(WRITE "${source_package_manifest}" "${source_package_manifest_content}\n")

set(forbidden_source_paths
    ".gitattributes"
    "src/ai"
    "src/apps/codex-backend"
    "src/apps/codex-backend-client"
    "docs/ai/openai/codex"
    "tools/codex"
    "tests/component/codex"
    "tests/installed/codex"
    "tests/policy/codex"
    "tests/policy/security/CodexSyntheticSecretLeakGuardTest.py"
    "tests/CodexSourcePackageTest.cmake"
    "tests/CodexBinaryPackageTest.cmake"
)
foreach(forbidden_source_path IN LISTS forbidden_source_paths)
    if(EXISTS "${source_root}/${forbidden_source_path}")
        message(
            FATAL_ERROR
                "removed SNode.C ownership path entered source package: ${forbidden_source_path}"
        )
    endif()
endforeach()

foreach(source_package_file IN LISTS source_package_files)
    if(source_package_file
       MATCHES
       "(^|/)(app-server-schema|app-server-surface|app-server-protocol-source|app-server-evidence|app-server-fixtures)(/|$)"
       OR source_package_file
          MATCHES
          "(^|/)app_server_(a1_[1-4]|a1_shared|contracts|fixtures|schema_paths|surface)\\.py$"
    )
        message(
            FATAL_ERROR
                "transferred protocol input or generator entered source package: ${source_package_file}"
        )
    endif()
    if(source_package_file MATCHES
       "(^|/)(\\.git|_CPack_Packages|__pycache__)(/|$)"
       OR source_package_file MATCHES "\\.py[cod]$"
    )
        message(
            FATAL_ERROR
                "local execution metadata entered source package: ${source_package_file}"
        )
    endif()
endforeach()

set(required_cutover_source_files
    "docs/migrations/aisuite-cutover-baseline-ctest.json"
    "docs/migrations/aisuite-cutover-boundary.json"
    "docs/migrations/aisuite-cutover-boundary.md"
    "docs/migrations/aisuite-cutover-plan.json"
    "docs/migrations/aisuite-cutover-start-state.json"
    "docs/migrations/codex-to-aisuite.md"
    "tests/AISuiteCutoverBinaryPackageTest.cmake"
    "tests/AISuiteCutoverSourcePackageTest.cmake"
    "tests/ownership/AISuiteCutoverMutationTest.py"
    "tests/ownership/AISuiteCutoverStartStateMutationTest.py"
    "tests/ownership/CMakeLists.txt"
    "tools/ownership/verify_aisuite_cutover.py"
)
foreach(required_cutover_source_file IN LISTS required_cutover_source_files)
    if(NOT EXISTS "${source_root}/${required_cutover_source_file}")
        message(
            FATAL_ERROR
                "cutover source-package input is missing: ${required_cutover_source_file}"
        )
    endif()
endforeach()

if(EXISTS "${source_root}/.git")
    message(FATAL_ERROR "source package unexpectedly contains .git")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env "HOME=${empty_home}" "CMAKE_PREFIX_PATH="
        "GIT_DIR=${source_root}/.git-forbidden"
        "GIT_CEILING_DIRECTORIES=${extracted_stage}"
        "GIT_CONFIG_GLOBAL=/dev/null" "GIT_CONFIG_NOSYSTEM=1"
        "http_proxy=http://127.0.0.1:9" "https_proxy=http://127.0.0.1:9"
        "ALL_PROXY=http://127.0.0.1:9" "NO_PROXY="
        "CMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE"
        "CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE"
        "CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE"
        "CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=TRUE"
        "PYTHONDONTWRITEBYTECODE=1" "${AISUITE_CUTOVER_PYTHON}" -I -B
        "${source_root}/tools/ownership/verify_aisuite_cutover.py" check-package
        --repo-root "${source_root}"
    WORKING_DIRECTORY "${source_root}"
    RESULT_VARIABLE package_check_result
    OUTPUT_VARIABLE package_check_output
    ERROR_VARIABLE package_check_error
)
if(NOT package_check_result EQUAL 0)
    message(
        FATAL_ERROR
            "package-safe AISuite cutover check failed\n${package_check_output}\n${package_check_error}"
    )
endif()

set(closure_checker
    "${source_root}/tools/ownership/verify_aisuite_cutover_closure.py"
)
if(NOT EXISTS "${closure_checker}")
    message(FATAL_ERROR "source package lacks AISuite closure checker")
endif()
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env "HOME=${empty_home}" "CMAKE_PREFIX_PATH="
        "GIT_DIR=${source_root}/.git-forbidden"
        "GIT_CEILING_DIRECTORIES=${extracted_stage}"
        "GIT_CONFIG_GLOBAL=/dev/null" "GIT_CONFIG_NOSYSTEM=1"
        "http_proxy=http://127.0.0.1:9" "https_proxy=http://127.0.0.1:9"
        "ALL_PROXY=http://127.0.0.1:9" "NO_PROXY="
        "CMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE"
        "CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE"
        "CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE"
        "CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=TRUE"
        "PYTHONDONTWRITEBYTECODE=1" "${AISUITE_CUTOVER_PYTHON}" -I -B
        "${closure_checker}" check-package --repo-root "${source_root}"
    WORKING_DIRECTORY "${source_root}"
    RESULT_VARIABLE closure_check_result
    OUTPUT_VARIABLE closure_check_output
    ERROR_VARIABLE closure_check_error
)
if(NOT closure_check_result EQUAL 0)
    message(
        FATAL_ERROR
            "package-safe AISuite closure check failed\n${closure_check_output}\n${closure_check_error}"
    )
endif()

list(LENGTH source_package_files source_package_file_count)
message(
    STATUS
        "AISuite cutover source package verified: files=${source_package_file_count}, boundary=passed, closure=passed"
)
