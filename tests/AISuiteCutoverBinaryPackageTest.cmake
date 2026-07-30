cmake_minimum_required(VERSION 3.18)

if(NOT DEFINED SNODEC_BUILD_DIR
   OR NOT DEFINED SNODEC_SOURCE_DIR
   OR NOT DEFINED CMAKE_CPACK_COMMAND
   OR NOT DEFINED CMAKE_CXX_COMPILER
)
    message(
        FATAL_ERROR
            "SNODEC_BUILD_DIR, SNODEC_SOURCE_DIR, CMAKE_CPACK_COMMAND, and CMAKE_CXX_COMPILER are required"
    )
endif()

find_program(AISUITE_CUTOVER_PYTHON NAMES python3 REQUIRED)

set(aisuite_cutover_tool
    "${SNODEC_SOURCE_DIR}/tools/ownership/verify_aisuite_cutover.py"
)
set(cpack_config "${SNODEC_BUILD_DIR}/CPackConfig.cmake")
set(binary_package_stage
    "${SNODEC_BUILD_DIR}/aisuite-cutover-binary-package-audit"
)
set(extracted_stage "${binary_package_stage}/extracted")
set(binary_package_manifest "${binary_package_stage}/binary-manifest.txt")

file(REMOVE_RECURSE "${binary_package_stage}")
file(MAKE_DIRECTORY "${binary_package_stage}" "${extracted_stage}")

file(READ "${cpack_config}" cpack_config_content)
string(REGEX MATCH "set\\(CPACK_COMPONENTS_ALL \"([^\"]*)\"\\)"
             cpack_components_match "${cpack_config_content}"
)
if("${cpack_components_match}" STREQUAL "")
    message(FATAL_ERROR "CPackConfig.cmake lacks CPACK_COMPONENTS_ALL")
endif()
set(cpack_components "${CMAKE_MATCH_1}")
list(LENGTH cpack_components cpack_component_count)
if(NOT cpack_component_count EQUAL 66)
    message(
        FATAL_ERROR
            "complete CPack component count changed: expected 66, found ${cpack_component_count}"
    )
endif()

set(removed_components ai-openai-codex ai-openai-codex-backend
                       ai-openai-codex-frontend
)
foreach(removed_component IN LISTS removed_components)
    if(removed_component IN_LIST cpack_components)
        message(
            FATAL_ERROR
                "removed component remains in CPackConfig.cmake: ${removed_component}"
        )
    endif()
endforeach()

string(REGEX MATCH "set\\(CPACK_COMPONENT_APPS_DEPENDS ([^\\)]*)\\)"
             cpack_apps_dependencies_match "${cpack_config_content}"
)
if("${cpack_apps_dependencies_match}" STREQUAL "")
    message(FATAL_ERROR "CPackConfig.cmake lacks CPACK_COMPONENT_APPS_DEPENDS")
endif()
set(cpack_apps_dependencies "${CMAKE_MATCH_1}")
string(REGEX REPLACE "[\r\n\t ]+" ";" cpack_apps_dependencies
                     "${cpack_apps_dependencies}"
)
list(FILTER cpack_apps_dependencies EXCLUDE REGEX "^$")
list(LENGTH cpack_apps_dependencies cpack_apps_dependency_count)
if(NOT cpack_apps_dependency_count EQUAL 17)
    message(
        FATAL_ERROR
            "apps CPack dependency count changed: expected 17, found ${cpack_apps_dependency_count}"
    )
endif()
if(ai-openai-codex-frontend IN_LIST cpack_apps_dependencies)
    message(
        FATAL_ERROR
            "removed Codex frontend dependency remains on apps component"
    )
endif()

execute_process(
    COMMAND
        "${CMAKE_CPACK_COMMAND}" -G TGZ -C Debug --config "${cpack_config}" -D
        "CPACK_OUTPUT_FILE_PREFIX=${binary_package_stage}" -D
        "CPACK_PACKAGING_INSTALL_PREFIX=/" -D
        "CPACK_ARCHIVE_COMPONENT_INSTALL=OFF" -D "CPACK_MONOLITHIC_INSTALL=ON"
    WORKING_DIRECTORY "${SNODEC_BUILD_DIR}"
    RESULT_VARIABLE package_result
    OUTPUT_VARIABLE package_output
    ERROR_VARIABLE package_error
)
if(NOT package_result EQUAL 0)
    message(
        FATAL_ERROR
            "AISuite cutover binary-package generation failed\n${package_output}\n${package_error}"
    )
endif()

file(
    GLOB binary_archives
    LIST_DIRECTORIES FALSE
    "${binary_package_stage}/*.tar.gz"
)
list(SORT binary_archives)
list(LENGTH binary_archives binary_archive_count)
if(NOT binary_archive_count EQUAL 1)
    message(
        FATAL_ERROR
            "expected exactly one fresh complete TGZ binary package, found ${binary_archive_count}: ${binary_archives}"
    )
endif()
list(GET binary_archives 0 binary_archive)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar tf "${binary_archive}"
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE archive_entries
    ERROR_VARIABLE list_error
)
if(NOT list_result EQUAL 0)
    message(
        FATAL_ERROR
            "could not list AISuite cutover binary package ${binary_archive}\n${list_error}"
    )
endif()
string(REPLACE "\r\n" "\n" archive_entries "${archive_entries}")
string(REPLACE "\n" ";" archive_entry_lines "${archive_entries}")
foreach(archive_entry IN LISTS archive_entry_lines)
    if(archive_entry MATCHES "(^|/)\\.\\.(/|$)")
        message(
            FATAL_ERROR
                "unsafe parent traversal in binary package: ${archive_entry}"
        )
    endif()
    string(TOLOWER "${archive_entry}" archive_entry_lower)
    if(archive_entry MATCHES "(^|/)include/snode\\.c/ai(/|$)"
       OR archive_entry MATCHES "(^|/)lib[^/]*/libsnodec-ai-openai-codex"
       OR archive_entry MATCHES "(^|/)bin/codex-backend(-client)?/?$"
       OR (archive_entry MATCHES "(^|/)lib[^/]*/cmake/snodec/"
           AND archive_entry_lower MATCHES
               "(ai-openai-codex|codex-backend|codex-backend-client|codex)")
    )
        message(
            FATAL_ERROR
                "removed Codex file or directory entered binary package: ${archive_entry}"
        )
    endif()
    if(archive_entry MATCHES "^[^/]+/(src|tests|tools|docs)(/|$)")
        message(
            FATAL_ERROR
                "source-only file or directory entered binary package: ${archive_entry}"
        )
    endif()
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xzf "${binary_archive}"
    WORKING_DIRECTORY "${extracted_stage}"
    RESULT_VARIABLE extract_result
    OUTPUT_VARIABLE extract_output
    ERROR_VARIABLE extract_error
)
if(NOT extract_result EQUAL 0)
    message(
        FATAL_ERROR
            "could not extract AISuite cutover binary package ${binary_archive}\n${extract_output}\n${extract_error}"
    )
endif()

file(
    GLOB_RECURSE snodec_config_files
    LIST_DIRECTORIES FALSE
    "${extracted_stage}/*/lib*/cmake/snodec/snodecConfig.cmake"
)
list(LENGTH snodec_config_files snodec_config_file_count)
if(NOT snodec_config_file_count EQUAL 1)
    message(
        FATAL_ERROR
            "expected one packaged snodecConfig.cmake, found ${snodec_config_file_count}: ${snodec_config_files}"
    )
endif()
list(GET snodec_config_files 0 snodec_config_file)
get_filename_component(snodec_config_dir "${snodec_config_file}" DIRECTORY)
get_filename_component(cmake_package_dir "${snodec_config_dir}" DIRECTORY)
get_filename_component(package_libdir "${cmake_package_dir}" DIRECTORY)
get_filename_component(package_root "${package_libdir}" DIRECTORY)

file(
    GLOB_RECURSE packaged_cmake_metadata
    LIST_DIRECTORIES FALSE
    "${package_root}/lib*/cmake/snodec/*.cmake"
)
foreach(packaged_cmake_file IN LISTS packaged_cmake_metadata)
    file(READ "${packaged_cmake_file}" packaged_cmake_content)
    string(TOLOWER "${packaged_cmake_content}" packaged_cmake_content_lower)
    if(packaged_cmake_content_lower
       MATCHES
       "(ai-openai-codex|codex-backend|codex-backend-client|snodec::ai-openai|ai/openai/codex)"
    )
        file(RELATIVE_PATH packaged_cmake_relative "${package_root}"
             "${packaged_cmake_file}"
        )
        message(
            FATAL_ERROR
                "removed Codex target or path remains in packaged CMake metadata: ${packaged_cmake_relative}"
        )
    endif()
endforeach()

file(
    GLOB_RECURSE binary_package_files
    LIST_DIRECTORIES FALSE
    RELATIVE "${package_root}"
    "${package_root}/*"
)
list(SORT binary_package_files)
set(normalized_binary_package_files)
foreach(binary_package_file IN LISTS binary_package_files)
    string(REPLACE "\\" "/" binary_package_file "${binary_package_file}")
    if(binary_package_file MATCHES "^\\./")
        string(SUBSTRING "${binary_package_file}" 2 -1 binary_package_file)
    endif()
    list(APPEND normalized_binary_package_files "${binary_package_file}")
endforeach()
set(binary_package_files "${normalized_binary_package_files}")
list(REMOVE_DUPLICATES binary_package_files)
string(REPLACE ";" "\n" binary_package_manifest_content
               "${binary_package_files}"
)
file(WRITE "${binary_package_manifest}" "${binary_package_manifest_content}\n")

foreach(binary_package_file IN LISTS binary_package_files)
    if(binary_package_file MATCHES "^include/snode\\.c/ai(/|$)"
       OR binary_package_file MATCHES "^lib[^/]*/libsnodec-ai-openai-codex"
       OR binary_package_file MATCHES "^bin/codex-backend(-client)?$"
       OR (binary_package_file MATCHES "^lib[^/]*/cmake/snodec/"
           AND binary_package_file MATCHES
               "(ai-openai-codex|codex-backend|codex-backend-client|[Cc]odex)")
    )
        message(
            FATAL_ERROR
                "removed Codex binary artifact entered package: ${binary_package_file}"
        )
    endif()
    if(binary_package_file MATCHES "^(src|tests|tools|docs)/")
        message(
            FATAL_ERROR
                "source-only cutover artifact entered binary package: ${binary_package_file}"
        )
    endif()
endforeach()

function(require_packaged_path expected_path)
    list(FIND binary_package_files "${expected_path}" expected_index)
    if(expected_index EQUAL -1)
        message(
            FATAL_ERROR
                "representative retained package path is missing: ${expected_path}"
        )
    endif()
endfunction()

function(require_one_packaged_match expected_pattern description)
    set(matches)
    foreach(binary_package_file IN LISTS binary_package_files)
        if(binary_package_file MATCHES "${expected_pattern}")
            list(APPEND matches "${binary_package_file}")
        endif()
    endforeach()
    list(LENGTH matches match_count)
    if(NOT match_count EQUAL 1)
        message(
            FATAL_ERROR
                "expected one packaged ${description}, found ${match_count}: ${matches}"
        )
    endif()
endfunction()

require_packaged_path("include/snode.c/core/socket/stream/SocketClient.h")
require_packaged_path("include/snode.c/net/un/stream/legacy/SocketClient.h")
require_packaged_path("bin/echoclient-legacy-un")
require_one_packaged_match(
    "^lib[^/]*/libsnodec-core\\.so\\.1$" "core SOVERSION library"
)
require_one_packaged_match(
    "^lib[^/]*/libsnodec-net-un-stream-legacy\\.so\\.1$"
    "Unix legacy stream SOVERSION library"
)
require_one_packaged_match(
    "^lib[^/]*/cmake/snodec/snodecConfig\\.cmake$" "SNode.C package config"
)
require_one_packaged_match(
    "^lib[^/]*/cmake/snodec/snodec_core_Targets\\.cmake$" "core target export"
)
require_one_packaged_match(
    "^lib[^/]*/cmake/snodec/snodec_net-un-stream-legacy_Targets\\.cmake$"
    "Unix legacy stream target export"
)

execute_process(
    COMMAND
        "${AISUITE_CUTOVER_PYTHON}" -I -B "${aisuite_cutover_tool}" check
        --repo-root "${SNODEC_SOURCE_DIR}" --cpack-config "${cpack_config}"
        --binary-package-manifest "${binary_package_manifest}" --install-root
        "${package_root}"
    WORKING_DIRECTORY "${SNODEC_SOURCE_DIR}"
    RESULT_VARIABLE boundary_check_result
    OUTPUT_VARIABLE boundary_check_output
    ERROR_VARIABLE boundary_check_error
)
if(NOT boundary_check_result EQUAL 0)
    message(
        FATAL_ERROR
            "binary-package ownership-boundary check failed\n${boundary_check_output}\n${boundary_check_error}"
    )
endif()

set(consumer_source "${binary_package_stage}/consumer")
set(consumer_build "${binary_package_stage}/consumer-build")
file(MAKE_DIRECTORY "${consumer_source}")
file(
    WRITE "${consumer_source}/main.cpp"
    "#include <core/socket/stream/SocketClient.h>\n"
    "#include <net/un/stream/legacy/SocketClient.h>\n"
    "int main() { return 0; }\n"
)
file(
    WRITE "${consumer_source}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.18)\n"
    "project(SNodeCBinaryPackageConsumer LANGUAGES CXX)\n"
    "find_package(snodec CONFIG REQUIRED COMPONENTS core net-un-stream-legacy)\n"
    "add_executable(binary-package-consumer main.cpp)\n"
    "target_compile_features(binary-package-consumer PRIVATE cxx_std_20)\n"
    "target_link_libraries(binary-package-consumer PRIVATE snodec::core snodec::net-un-stream-legacy)\n"
)
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -S "${consumer_source}" -B "${consumer_build}"
        "-Dsnodec_DIR=${snodec_config_dir}"
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE
        -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE
        -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE
        -DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=TRUE
    RESULT_VARIABLE consumer_configure_result
    OUTPUT_VARIABLE consumer_configure_output
    ERROR_VARIABLE consumer_configure_error
)
if(NOT consumer_configure_result EQUAL 0)
    message(
        FATAL_ERROR
            "binary-package consumer configure failed\n${consumer_configure_output}\n${consumer_configure_error}"
    )
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}"
    RESULT_VARIABLE consumer_build_result
    OUTPUT_VARIABLE consumer_build_output
    ERROR_VARIABLE consumer_build_error
)
if(NOT consumer_build_result EQUAL 0)
    message(
        FATAL_ERROR
            "binary-package consumer build failed\n${consumer_build_output}\n${consumer_build_error}"
    )
endif()
execute_process(
    COMMAND "${consumer_build}/binary-package-consumer"
    RESULT_VARIABLE consumer_run_result
    OUTPUT_VARIABLE consumer_run_output
    ERROR_VARIABLE consumer_run_error
)
if(NOT consumer_run_result EQUAL 0)
    message(
        FATAL_ERROR
            "binary-package consumer execution failed\n${consumer_run_output}\n${consumer_run_error}"
    )
endif()

set(removed_component_source
    "${binary_package_stage}/removed-component-consumer"
)
file(MAKE_DIRECTORY "${removed_component_source}")
file(
    WRITE "${removed_component_source}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.18)\n"
    "project(SNodeCRemovedBinaryComponentConsumer LANGUAGES NONE)\n"
    "find_package(snodec CONFIG REQUIRED COMPONENTS \"\${REQUESTED_COMPONENT}\")\n"
)
foreach(removed_component IN LISTS removed_components)
    set(removed_component_build
        "${binary_package_stage}/removed-${removed_component}-build"
    )
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -S "${removed_component_source}" -B
            "${removed_component_build}"
            "-DREQUESTED_COMPONENT=${removed_component}"
            "-Dsnodec_DIR=${snodec_config_dir}"
            -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE
            -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE
            -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE
            -DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=TRUE
        RESULT_VARIABLE removed_configure_result
        OUTPUT_VARIABLE removed_configure_output
        ERROR_VARIABLE removed_configure_error
    )
    if(removed_configure_result EQUAL 0)
        message(
            FATAL_ERROR
                "removed binary-package component unexpectedly configured: ${removed_component}"
        )
    endif()
    set(expected_not_found_message
        "Unsupported SNode.C component requested: ${removed_component}"
    )
    set(removed_configure_log
        "${removed_configure_output}\n${removed_configure_error}"
    )
    string(FIND "${removed_configure_log}" "${expected_not_found_message}"
                not_found_message_index
    )
    if(not_found_message_index EQUAL -1)
        message(
            FATAL_ERROR
                "removed binary-package component failed for an unexpected reason: ${removed_component}\n${removed_configure_log}"
        )
    endif()
endforeach()

list(LENGTH binary_package_files binary_package_file_count)
if(NOT binary_package_file_count EQUAL 841)
    message(
        FATAL_ERROR
            "binary-package retained manifest count changed: expected 841, found ${binary_package_file_count}"
    )
endif()
message(
    STATUS
        "AISuite cutover binary package verified: components=66, apps_dependencies=17, files=841"
)
