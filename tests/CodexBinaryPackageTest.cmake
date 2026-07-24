if(NOT DEFINED SNODEC_BUILD_DIR OR NOT DEFINED CMAKE_CPACK_COMMAND)
    message(
        FATAL_ERROR
            "SNODEC_BUILD_DIR and CMAKE_CPACK_COMMAND are required"
    )
endif()

set(binary_package_stage
    "${SNODEC_BUILD_DIR}/codex-binary-package-audit"
)
file(REMOVE_RECURSE "${binary_package_stage}")
file(MAKE_DIRECTORY "${binary_package_stage}")

set(
    codex_components
    "ai-openai-codex;ai-openai-codex-backend;ai-openai-codex-frontend"
)
execute_process(
    COMMAND
        "${CMAKE_CPACK_COMMAND}" -G TGZ -C Debug --config
        "${SNODEC_BUILD_DIR}/CPackConfig.cmake" -D
        "CPACK_OUTPUT_FILE_PREFIX=${binary_package_stage}" -D
        "CPACK_ARCHIVE_COMPONENT_INSTALL=ON" -D
        "CPACK_COMPONENTS_ALL=${codex_components}" -D
        "CPACK_COMPONENT_AI-OPENAI-CODEX_DEPENDS=" -D
        "CPACK_COMPONENT_AI-OPENAI-CODEX-BACKEND_DEPENDS=" -D
        "CPACK_COMPONENT_AI-OPENAI-CODEX-FRONTEND_DEPENDS="
    WORKING_DIRECTORY "${SNODEC_BUILD_DIR}"
    RESULT_VARIABLE package_result
    OUTPUT_VARIABLE package_output
    ERROR_VARIABLE package_error
)
if(NOT package_result EQUAL 0)
    message(
        FATAL_ERROR
            "Codex binary-package generation failed\n${package_output}\n${package_error}"
    )
endif()

file(GLOB binary_archives "${binary_package_stage}/*.tar.gz")
list(SORT binary_archives)
list(LENGTH binary_archives binary_archive_count)
if(NOT binary_archive_count EQUAL 3)
    message(
        FATAL_ERROR
            "expected exactly three Codex component TGZ archives, found ${binary_archive_count}: ${binary_archives}"
    )
endif()

set(
    expected_codex_public_headers
    "AppServerClient.h"
    "Protocol.h"
    "backend/BackendCommand.h"
    "backend/BackendCore.h"
    "backend/BackendEvent.h"
    "backend/BackendState.h"
    "backend/FrontendSession.h"
    "backend/Reducer.h"
    "backend/Snapshot.h"
    "frontend/BackendAdapter.h"
    "frontend/Codec.h"
    "frontend/EventCoalescer.h"
    "frontend/EventJournal.h"
    "frontend/Messages.h"
    "frontend/Protocol.h"
    "frontend/UpdateBatch.h"
    "stdio/Client.h"
    "typed/Accounts.h"
    "typed/Client.h"
    "typed/CodexErrorInfo.h"
    "typed/Configuration.h"
    "typed/Conversation.h"
    "typed/Events.h"
    "typed/Items.h"
    "typed/Models.h"
    "typed/Results.h"
    "typed/ServerRequests.h"
    "typed/Threads.h"
    "typed/Turns.h"
    "typed/Types.h"
)
list(SORT expected_codex_public_headers)

set(
    expected_soversion_libraries
    "libsnodec-ai-openai-codex-backend.so.1"
    "libsnodec-ai-openai-codex-frontend.so.1"
    "libsnodec-ai-openai-codex.so.1"
)
list(SORT expected_soversion_libraries)

set(
    forbidden_binary_patterns
    "/ai/openai/codex/detail/"
    "/tools/"
    "/tests/"
    "/docs/"
    "/app-server-schema/"
    "/app-server-surface/"
    "/app-server-evidence/"
    "/app-server-fixtures/"
    "/app-server-protocol-source/"
    "\\.cpp$"
    "\\.inc$"
    "\\.json$"
    "\\.py$"
)

set(observed_codex_public_headers)
set(observed_soversion_libraries)
set(observed_components)
foreach(binary_archive IN LISTS binary_archives)
    get_filename_component(binary_archive_name "${binary_archive}" NAME)
    set(matched_component)
    foreach(component IN LISTS codex_components)
        if(binary_archive_name MATCHES "-${component}\\.tar\\.gz$")
            if(NOT "${matched_component}" STREQUAL "")
                message(
                    FATAL_ERROR
                        "Codex archive matches multiple components: ${binary_archive_name}"
                )
            endif()
            set(matched_component "${component}")
        endif()
    endforeach()
    if("${matched_component}" STREQUAL "")
        message(
            FATAL_ERROR
                "unexpected Codex component archive name: ${binary_archive_name}"
        )
    endif()
    list(APPEND observed_components "${matched_component}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar tf "${binary_archive}"
        RESULT_VARIABLE list_result
        OUTPUT_VARIABLE archive_entries
        ERROR_VARIABLE list_error
    )
    if(NOT list_result EQUAL 0)
        message(
            FATAL_ERROR
                "could not list Codex binary package ${binary_archive}\n${list_error}"
        )
    endif()
    string(REPLACE "\r\n" "\n" archive_entries "${archive_entries}")
    string(REPLACE "\n" ";" archive_entry_lines "${archive_entries}")

    foreach(archive_entry IN LISTS archive_entry_lines)
        if(archive_entry STREQUAL "" OR archive_entry MATCHES "/$")
            continue()
        endif()
        foreach(forbidden_pattern IN LISTS forbidden_binary_patterns)
            if(archive_entry MATCHES "${forbidden_pattern}")
                message(
                    FATAL_ERROR
                        "private/source-only Codex artifact entered ${binary_archive_name}: ${archive_entry}"
                )
            endif()
        endforeach()

        if(archive_entry MATCHES
           "(^|/)include/snode\\.c/ai/openai/codex/(.+\\.h)$")
            list(APPEND observed_codex_public_headers "${CMAKE_MATCH_2}")
        endif()

        get_filename_component(archive_entry_name "${archive_entry}" NAME)
        if(archive_entry_name MATCHES
           "^libsnodec-ai-openai-codex(-backend|-frontend)?\\.so\\.1$")
            list(APPEND
                 observed_soversion_libraries "${archive_entry_name}")
        endif()
    endforeach()
endforeach()

list(SORT observed_components)
list(SORT codex_components)
if(NOT "${observed_components}" STREQUAL "${codex_components}")
    message(
        FATAL_ERROR
            "Codex binary component set changed\nexpected: ${codex_components}\nobserved: ${observed_components}"
    )
endif()

list(SORT observed_codex_public_headers)
if(NOT "${observed_codex_public_headers}" STREQUAL
   "${expected_codex_public_headers}")
    message(
        FATAL_ERROR
            "Codex binary-package public-header inventory changed\nexpected: ${expected_codex_public_headers}\nobserved: ${observed_codex_public_headers}"
    )
endif()

list(SORT observed_soversion_libraries)
if(NOT "${observed_soversion_libraries}" STREQUAL
   "${expected_soversion_libraries}")
    message(
        FATAL_ERROR
            "Codex binary-package SOVERSION inventory changed\nexpected: ${expected_soversion_libraries}\nobserved: ${observed_soversion_libraries}"
    )
endif()

message(
    STATUS
        "Codex binary packages: archives=3, public_headers=30, soversion_libraries=3"
)
