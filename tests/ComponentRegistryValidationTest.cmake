# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2026
#
# This file may be used under the terms of the GNU Lesser General Public
# License version 3 or later, or under the MIT License.

cmake_minimum_required(VERSION 3.18)

if(NOT DEFINED SNODEC_SOURCE_DIR)
    message(FATAL_ERROR "SNODEC_SOURCE_DIR is required")
endif()
if(NOT DEFINED SNODEC_TEST_BINARY_DIR)
    message(FATAL_ERROR "SNODEC_TEST_BINARY_DIR is required")
endif()


# Production components must use the central registration API. Raw install()
# rules in src/ would bypass the runtime/development split, and CPack metadata
# must never be reconstructed in the global packaging policy file.
file(GLOB_RECURSE component_cmake_files "${SNODEC_SOURCE_DIR}/src/*/CMakeLists.txt")
foreach(component_cmake_file IN LISTS component_cmake_files)
    file(READ "${component_cmake_file}" component_cmake_source)
    if(component_cmake_source MATCHES "(^|\n)[ \t]*install[ \t]*[(]")
        message(
            FATAL_ERROR
                "Raw install() rule found in '${component_cmake_file}'; use the SNode.C component helpers"
        )
    endif()
    if(component_cmake_source MATCHES "cpack_add_component[ \t]*[(]")
        message(
            FATAL_ERROR
                "Local CPack call found in '${component_cmake_file}'; component helpers own CPack registration"
        )
    endif()
endforeach()
file(READ "${SNODEC_SOURCE_DIR}/cmake/Packing.cmake" packing_source)
if(packing_source MATCHES "cpack_add_component[ \t]*[(]")
    message(
        FATAL_ERROR
            "Packing.cmake must contain policy only; components register in their own CMakeLists.txt files"
    )
endif()

set(stage "${SNODEC_TEST_BINARY_DIR}/component-registry-validation")
file(REMOVE_RECURSE "${stage}")
file(MAKE_DIRECTORY "${stage}")

function(run_registry_case case_name case_body expected_result expected_message)
    set(source_dir "${stage}/${case_name}")
    set(binary_dir "${source_dir}/build")
    file(MAKE_DIRECTORY "${source_dir}")
    file(WRITE "${source_dir}/component.cpp" "int component_value() { return 1; }\n")
    file(
        WRITE "${source_dir}/component.h"
        "int component_value();\n"
    )
    file(
        WRITE "${source_dir}/CMakeLists.txt"
        "cmake_minimum_required(VERSION 3.18)\n"
        "project(${case_name} VERSION 1.0.0 LANGUAGES CXX)\n"
        "list(APPEND CMAKE_MODULE_PATH \"${SNODEC_SOURCE_DIR}/cmake\")\n"
        "include(SNodeCComponent)\n"
        "${case_body}\n"
    )

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -S "${source_dir}" -B "${binary_dir}"
        RESULT_VARIABLE configure_result
        OUTPUT_VARIABLE configure_output
        ERROR_VARIABLE configure_error
    )
    set(configure_log "${configure_output}\n${configure_error}")

    if(expected_result STREQUAL "SUCCESS")
        if(NOT configure_result EQUAL 0)
            message(
                FATAL_ERROR
                    "Registry case '${case_name}' unexpectedly failed:\n${configure_log}"
            )
        endif()
    elseif(expected_result STREQUAL "FAILURE")
        if(configure_result EQUAL 0)
            message(
                FATAL_ERROR
                    "Registry case '${case_name}' unexpectedly succeeded"
            )
        endif()
        string(FIND "${configure_log}" "${expected_message}" message_position)
        if(message_position EQUAL -1)
            message(
                FATAL_ERROR
                    "Registry case '${case_name}' did not report '${expected_message}':\n${configure_log}"
            )
        endif()
    else()
        message(FATAL_ERROR "Unknown expected result '${expected_result}'")
    endif()
endfunction()

set(valid_case [=[
add_library(a SHARED component.cpp)
snodec_add_component(TARGET a)
snodec_install_component_headers(
    COMPONENT a FILES component.h DESTINATION include
)
snodec_finalize_cpack_components()
]=])
run_registry_case(valid "${valid_case}" SUCCESS "")

set(duplicate_case [=[
add_library(a SHARED component.cpp)
add_library(b SHARED component.cpp)
snodec_add_component(TARGET a COMPONENT duplicate)
snodec_install_component_headers(
    COMPONENT duplicate FILES component.h DESTINATION include
)
snodec_add_component(TARGET b COMPONENT duplicate)
]=])
run_registry_case(
    duplicate
    "${duplicate_case}"
    FAILURE
    "SNode.C component 'duplicate' is registered twice"
)

set(missing_case [=[
add_library(a SHARED component.cpp)
snodec_add_component(TARGET a DEPENDS missing)
snodec_install_component_headers(
    COMPONENT a FILES component.h DESTINATION include
)
snodec_finalize_cpack_components()
]=])
run_registry_case(
    missing
    "${missing_case}"
    FAILURE
    "SNode.C component 'a' depends on unregistered component 'missing'"
)

set(runtime_cycle_case [=[
add_library(a SHARED component.cpp)
add_library(b SHARED component.cpp)
snodec_add_component(TARGET a DEPENDS b)
snodec_install_component_headers(
    COMPONENT a FILES component.h DESTINATION include
)
snodec_add_component(TARGET b DEPENDS a)
snodec_install_component_headers(
    COMPONENT b FILES component.h DESTINATION include
)
snodec_finalize_cpack_components()
]=])
run_registry_case(
    runtime-cycle
    "${runtime_cycle_case}"
    FAILURE
    "Cyclic SNode.C runtime component dependency: a -> b -> a"
)

set(development_cycle_case [=[
add_library(a SHARED component.cpp)
add_library(b SHARED component.cpp)
snodec_add_component(TARGET a DEVELOPMENT_DEPENDS b)
snodec_install_component_headers(
    COMPONENT a FILES component.h DESTINATION include
)
snodec_add_component(TARGET b DEVELOPMENT_DEPENDS a)
snodec_install_component_headers(
    COMPONENT b FILES component.h DESTINATION include
)
snodec_finalize_cpack_components()
]=])
run_registry_case(
    development-cycle
    "${development_cycle_case}"
    FAILURE
    "Cyclic SNode.C development component dependency: a -> b -> a"
)


set(missing_value_case [=[
add_library(a SHARED component.cpp)
snodec_add_component(TARGET)
]=])
run_registry_case(
    missing-value
    "${missing_value_case}"
    FAILURE
    "snodec_add_component is missing values for: TARGET"
)
