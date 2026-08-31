# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2026
#
# This file may be used under the terms of the GNU Lesser General Public
# License version 3 or later, or under the MIT License.

cmake_minimum_required(VERSION 3.18)

if(NOT DEFINED PACKAGE_DIR)
    message(FATAL_ERROR "PACKAGE_DIR is required")
endif()
if(NOT DEFINED CPACK_CONFIG)
    message(FATAL_ERROR "CPACK_CONFIG is required")
endif()

find_program(DPKG_DEB_EXECUTABLE dpkg-deb REQUIRED)
include("${CPACK_CONFIG}")

file(GLOB deb_packages "${PACKAGE_DIR}/*.deb")
list(LENGTH deb_packages deb_package_count)
list(LENGTH CPACK_COMPONENTS_ALL expected_package_count)
if(NOT deb_package_count EQUAL expected_package_count)
    message(
        FATAL_ERROR
            "Expected ${expected_package_count} component packages, found ${deb_package_count} in '${PACKAGE_DIR}'"
    )
endif()

function(read_deb_field package_file field result)
    execute_process(
        COMMAND "${DPKG_DEB_EXECUTABLE}" --field "${package_file}" "${field}"
        RESULT_VARIABLE field_result
        OUTPUT_VARIABLE field_value
        ERROR_VARIABLE field_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT field_result EQUAL 0)
        message(
            FATAL_ERROR
                "Could not read ${field} from '${package_file}': ${field_error}"
        )
    endif()
    set(${result} "${field_value}" PARENT_SCOPE)
endfunction()

set(component_package_names)
foreach(deb_package IN LISTS deb_packages)
    read_deb_field("${deb_package}" Package package_name)
    list(APPEND component_package_names "${package_name}")
    string(MAKE_C_IDENTIFIER "${package_name}" package_key)
    set("DEB_PACKAGE_FILE_${package_key}" "${deb_package}")
endforeach()

function(component_package_name component result)
    if(DEFINED CPACK_DEBIAN_PACKAGE_NAME AND CPACK_DEBIAN_PACKAGE_NAME)
        set(package_base_name "${CPACK_DEBIAN_PACKAGE_NAME}")
    else()
        set(package_base_name "${CPACK_PACKAGE_NAME}")
    endif()
    string(TOLOWER "${package_base_name}-${component}" package_name)
    set(${result} "${package_name}" PARENT_SCOPE)
endfunction()

function(find_component_package component result_name result_file)
    component_package_name("${component}" expected_package_name)
    set(matches)
    foreach(package_name IN LISTS component_package_names)
        if(package_name STREQUAL expected_package_name)
            list(APPEND matches "${package_name}")
        endif()
    endforeach()

    list(LENGTH matches match_count)
    if(NOT match_count EQUAL 1)
        message(
            FATAL_ERROR
                "Expected exactly one Debian package '${expected_package_name}' for component '${component}', found: ${matches}"
        )
    endif()

    list(GET matches 0 package_name)
    string(MAKE_C_IDENTIFIER "${package_name}" package_key)
    set(${result_name} "${package_name}" PARENT_SCOPE)
    set(${result_file} "${DEB_PACKAGE_FILE_${package_key}}" PARENT_SCOPE)
endfunction()

function(assert_no_component_package component)
    component_package_name("${component}" unexpected_package_name)
    foreach(package_name IN LISTS component_package_names)
        if(package_name STREQUAL unexpected_package_name)
            message(
                FATAL_ERROR
                    "Unexpected Debian package '${package_name}' for component '${component}'"
            )
        endif()
    endforeach()
endfunction()

function(read_deb_contents package_file result)
    execute_process(
        COMMAND "${DPKG_DEB_EXECUTABLE}" --contents "${package_file}"
        RESULT_VARIABLE contents_result
        OUTPUT_VARIABLE contents
        ERROR_VARIABLE contents_error
    )
    if(NOT contents_result EQUAL 0)
        message(
            FATAL_ERROR
                "Could not inspect '${package_file}': ${contents_error}"
        )
    endif()
    set(${result} "${contents}" PARENT_SCOPE)
endfunction()

function(read_deb_dependency_names package_file result)
    read_deb_field("${package_file}" Depends package_dependencies)
    string(REPLACE "," ";" dependency_groups "${package_dependencies}")

    set(dependency_names)
    foreach(dependency_group IN LISTS dependency_groups)
        string(REPLACE "|" ";" alternatives "${dependency_group}")
        foreach(alternative IN LISTS alternatives)
            string(STRIP "${alternative}" alternative)
            if(alternative)
                string(REGEX REPLACE "[ \t(].*$" "" dependency_name
                                     "${alternative}"
                )
                list(APPEND dependency_names "${dependency_name}")
            endif()
        endforeach()
    endforeach()

    list(REMOVE_DUPLICATES dependency_names)
    set(${result} "${dependency_names}" PARENT_SCOPE)
endfunction()

function(assert_package_dependency component dependency_component)
    find_component_package("${component}" package_name package_file)
    find_component_package(
        "${dependency_component}" dependency_package_name
        dependency_package_file
    )
    read_deb_dependency_names("${package_file}" package_dependencies)
    if(NOT dependency_package_name IN_LIST package_dependencies)
        message(
            FATAL_ERROR
                "Package '${package_name}' does not depend on '${dependency_package_name}': ${package_dependencies}"
        )
    endif()
endfunction()

function(assert_no_package_dependency component dependency_component)
    find_component_package("${component}" package_name package_file)
    find_component_package(
        "${dependency_component}" dependency_package_name
        dependency_package_file
    )
    read_deb_dependency_names("${package_file}" package_dependencies)
    if(dependency_package_name IN_LIST package_dependencies)
        message(
            FATAL_ERROR
                "Package '${package_name}' unexpectedly depends on '${dependency_package_name}': ${package_dependencies}"
        )
    endif()
endfunction()

foreach(component IN LISTS CPACK_COMPONENTS_ALL)
    find_component_package("${component}" package_name package_file)
    read_deb_contents("${package_file}" package_contents)
    if(package_contents MATCHES "[./]usr/local/")
        message(
            FATAL_ERROR
                "Package '${package_name}' installs below /usr/local:\n${package_contents}"
        )
    endif()

    if(component MATCHES "-dev$")
        string(REGEX REPLACE "-dev$" "" runtime_component "${component}")
        assert_package_dependency("${component}" "${runtime_component}")

        if(package_contents MATCHES "[./]usr/include/|[./]usr/lib/.*/cmake/snodec/|[./]usr/lib/cmake/snodec/")
            # At least one development artifact is present. Individual key
            # components are checked more precisely below.
        else()
            message(
                FATAL_ERROR
                    "Development package '${package_name}' has no headers or CMake metadata:\n${package_contents}"
            )
        endif()
    endif()
endforeach()

foreach(package_name IN LISTS component_package_names)
    if(package_name MATCHES "sphy|mqtt-fast|unspecified")
        message(FATAL_ERROR "Unexpected obsolete package '${package_name}'")
    endif()
endforeach()

find_component_package(core core_package_name core_package_file)
find_component_package(core-dev core_dev_package_name core_dev_package_file)
read_deb_contents("${core_package_file}" core_contents)
read_deb_contents("${core_dev_package_file}" core_dev_contents)

if(NOT core_contents MATCHES "libsnodec-core\\.so\\.1")
    message(FATAL_ERROR "The core runtime package has no versioned library")
endif()
if(core_contents MATCHES "/include/|/cmake/snodec/|libsnodec-core\\.so ->")
    message(
        FATAL_ERROR
            "The core runtime package contains development artifacts:\n${core_contents}"
    )
endif()
if(NOT core_dev_contents MATCHES "/include/snode\\.c/core/")
    message(FATAL_ERROR "The core development package has no public headers")
endif()
if(NOT core_dev_contents MATCHES
       "/cmake/snodec/snodec_core_Targets\\.cmake"
)
    message(
        FATAL_ERROR
            "The core development package has no exported CMake target metadata"
    )
endif()
if(NOT core_dev_contents MATCHES "libsnodec-core\\.so ->")
    message(
        FATAL_ERROR
            "The core development package has no unversioned library namelink"
    )
endif()

set(core_runtime_dependencies_var "CPACK_COMPONENT_CORE_DEPENDS")
set(selected_multiplexer_components)
foreach(dependency IN LISTS ${core_runtime_dependencies_var})
    if(dependency MATCHES "^mux-")
        list(APPEND selected_multiplexer_components "${dependency}")
    endif()
endforeach()
list(LENGTH selected_multiplexer_components selected_multiplexer_count)
if(NOT selected_multiplexer_count EQUAL 1)
    message(
        FATAL_ERROR
            "core must depend on exactly one multiplexer component: "
            "${${core_runtime_dependencies_var}}"
    )
endif()
list(GET selected_multiplexer_components 0 selected_multiplexer_component)

assert_package_dependency(core "${selected_multiplexer_component}")
assert_package_dependency(core utils)
assert_package_dependency(core-dev core)
assert_package_dependency(core-dev utils-dev)
assert_no_package_dependency(
    core-dev "${selected_multiplexer_component}-dev"
)

assert_package_dependency(net-in6-phy-stream net-in6-phy)
assert_package_dependency(net-in6-phy-stream-dev net-in6-phy-stream)
assert_package_dependency(net-in6-phy-stream-dev net-in6-phy-dev)

assert_package_dependency(
    http-server-express-tls-in http-server-express
)
assert_package_dependency(
    http-server-express-tls-in net-in-stream-tls
)
assert_package_dependency(
    http-server-express-tls-in-dev http-server-express-dev
)
assert_package_dependency(
    http-server-express-tls-in-dev net-in-stream-tls-dev
)

assert_package_dependency(mqtt-client-websocket mqtt-client)
assert_package_dependency(mqtt-client-websocket websocket-client)
assert_package_dependency(mqtt-client-websocket-dev mqtt-client-dev)
assert_package_dependency(mqtt-client-websocket-dev websocket-client-dev)

find_component_package(apps apps_package_name apps_package_file)
find_component_package(
    snodec-control snodec_control_package_name snodec_control_package_file
)
assert_no_component_package(apps-dev)
assert_no_component_package(snodec-control-dev)

message(
    STATUS
        "Validated ${deb_package_count} SNode.C runtime/development component packages"
)
