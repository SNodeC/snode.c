# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# ---------------------------------------------------------------------------
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

include_guard(GLOBAL)

include(CMakeParseArguments)
include(CPackComponent)
include(GNUInstallDirs)

# Every component is declared where its target is declared. These helpers own
# the runtime/development split, CMake exports, CPack metadata, package
# dependencies, and the public find_package() component registry.

function(_snodec_component_key component result)
    string(MAKE_C_IDENTIFIER "${component}" component_key)
    string(TOUPPER "${component_key}" component_key)
    set(${result} "${component_key}" PARENT_SCOPE)
endfunction()

function(_snodec_validate_arguments prefix command_name)
    if(${prefix}_UNPARSED_ARGUMENTS)
        message(
            FATAL_ERROR
                "${command_name} received unknown arguments: ${${prefix}_UNPARSED_ARGUMENTS}"
        )
    endif()
    if(${prefix}_KEYWORDS_MISSING_VALUES)
        message(
            FATAL_ERROR
                "${command_name} is missing values for: ${${prefix}_KEYWORDS_MISSING_VALUES}"
        )
    endif()
endfunction()

function(_snodec_component_property component property result)
    _snodec_component_key("${component}" component_key)
    get_property(
        property_value GLOBAL
        PROPERTY "SNODEC_COMPONENT_${component_key}_${property}"
    )
    set(${result} "${property_value}" PARENT_SCOPE)
endfunction()

function(_snodec_set_component_property component property)
    _snodec_component_key("${component}" component_key)
    set_property(
        GLOBAL PROPERTY "SNODEC_COMPONENT_${component_key}_${property}" ${ARGN}
    )
endfunction()

function(_snodec_append_component_property component property)
    _snodec_component_key("${component}" component_key)
    set_property(
        GLOBAL APPEND
        PROPERTY "SNODEC_COMPONENT_${component_key}_${property}" ${ARGN}
    )
endfunction()

function(_snodec_record_install_component component)
    set_property(
        GLOBAL APPEND PROPERTY SNODEC_REFERENCED_INSTALL_COMPONENTS
                               "${component}"
    )
endfunction()

function(_snodec_record_shlibdeps_private_directory target)
    _snodec_real_target("${target}" real_target)
    get_target_property(target_type "${real_target}" TYPE)
    if(NOT target_type MATCHES "^(SHARED|MODULE)_LIBRARY$")
        return()
    endif()

    get_target_property(
        library_output_directory "${real_target}" LIBRARY_OUTPUT_DIRECTORY
    )
    get_target_property(target_binary_directory "${real_target}" BINARY_DIR)
    if(NOT library_output_directory
       OR library_output_directory STREQUAL
           "library_output_directory-NOTFOUND"
    )
        set(library_output_directory "${target_binary_directory}")
    elseif(NOT IS_ABSOLUTE "${library_output_directory}")
        get_filename_component(
            library_output_directory "${library_output_directory}" ABSOLUTE
            BASE_DIR "${target_binary_directory}"
        )
    endif()

    set_property(
        GLOBAL APPEND PROPERTY SNODEC_SHLIBDEPS_PRIVATE_DIRS
                               "${library_output_directory}"
    )
endfunction()

function(_snodec_real_target target result)
    get_target_property(aliased_target "${target}" ALIASED_TARGET)
    if(aliased_target
       AND NOT aliased_target STREQUAL "aliased_target-NOTFOUND"
    )
        set(${result} "${aliased_target}" PARENT_SCOPE)
    else()
        set(${result} "${target}" PARENT_SCOPE)
    endif()
endfunction()

function(_snodec_set_target_component target component)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Unknown SNode.C target '${target}'")
    endif()

    _snodec_real_target("${target}" real_target)
    get_property(
        component_is_set TARGET "${real_target}" PROPERTY SNODEC_COMPONENT SET
    )
    if(component_is_set)
        get_target_property(
            existing_component "${real_target}" SNODEC_COMPONENT
        )
        if(NOT existing_component STREQUAL component)
            message(
                FATAL_ERROR
                    "Target '${target}' already belongs to SNode.C component '${existing_component}'"
            )
        endif()
        return()
    endif()

    set_property(
        TARGET "${real_target}" PROPERTY SNODEC_COMPONENT "${component}"
    )
endfunction()

function(_snodec_register_component)
    set(options PUBLIC RUNTIME_ONLY)
    set(one_value_arguments COMPONENT DISPLAY_NAME DESCRIPTION)
    set(multi_value_arguments DEPENDS DEVELOPMENT_DEPENDS TARGETS)
    cmake_parse_arguments(
        ARG "${options}" "${one_value_arguments}" "${multi_value_arguments}"
        ${ARGN}
    )
    _snodec_validate_arguments(ARG "_snodec_register_component")

    if(NOT ARG_COMPONENT)
        message(FATAL_ERROR "COMPONENT is required")
    endif()
    if(ARG_COMPONENT MATCHES "-dev$")
        message(
            FATAL_ERROR
                "SNode.C base component '${ARG_COMPONENT}' uses the reserved '-dev' suffix"
        )
    endif()
    if(ARG_PUBLIC AND ARG_RUNTIME_ONLY)
        message(
            FATAL_ERROR
                "Runtime-only component '${ARG_COMPONENT}' cannot be a public find_package() component"
        )
    endif()
    if(NOT ARG_TARGETS)
        message(
            FATAL_ERROR
                "SNode.C component '${ARG_COMPONENT}' has no associated targets"
        )
    endif()

    get_property(registered_components GLOBAL PROPERTY SNODEC_BASE_COMPONENTS)
    if(ARG_COMPONENT IN_LIST registered_components)
        message(
            FATAL_ERROR
                "SNode.C component '${ARG_COMPONENT}' is registered twice"
        )
    endif()

    _snodec_component_key("${ARG_COMPONENT}" component_key)
    get_property(
        keyed_component GLOBAL
        PROPERTY "SNODEC_COMPONENT_${component_key}_NAME"
    )
    if(keyed_component AND NOT keyed_component STREQUAL ARG_COMPONENT)
        message(
            FATAL_ERROR
                "SNode.C components '${ARG_COMPONENT}' and '${keyed_component}' map to the same registry key"
        )
    endif()

    list(REMOVE_ITEM ARG_DEPENDS "${ARG_COMPONENT}")
    list(REMOVE_ITEM ARG_DEVELOPMENT_DEPENDS "${ARG_COMPONENT}")
    list(REMOVE_DUPLICATES ARG_DEPENDS)
    list(REMOVE_DUPLICATES ARG_DEVELOPMENT_DEPENDS)
    list(REMOVE_DUPLICATES ARG_TARGETS)

    set_property(
        GLOBAL APPEND PROPERTY SNODEC_BASE_COMPONENTS "${ARG_COMPONENT}"
    )
    _snodec_set_component_property("${ARG_COMPONENT}" NAME "${ARG_COMPONENT}")
    _snodec_set_component_property(
        "${ARG_COMPONENT}" DISPLAY_NAME "${ARG_DISPLAY_NAME}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" DESCRIPTION "${ARG_DESCRIPTION}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" RUNTIME_ONLY "${ARG_RUNTIME_ONLY}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" EXPLICIT_RUNTIME_DEPENDS ${ARG_DEPENDS}
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}"
        EXPLICIT_DEVELOPMENT_DEPENDS
        ${ARG_DEVELOPMENT_DEPENDS}
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" TARGETS ${ARG_TARGETS}
    )

    foreach(target IN LISTS ARG_TARGETS)
        _snodec_set_target_component("${target}" "${ARG_COMPONENT}")
    endforeach()

    if(ARG_PUBLIC)
        set_property(
            GLOBAL APPEND PROPERTY SNODEC_PUBLIC_COMPONENTS "${ARG_COMPONENT}"
        )
    endif()
endfunction()

function(_snodec_unwrap_link_item link_item result)
    set(unwrapped_link_item "${link_item}")
    set(previous_link_item)

    while(NOT unwrapped_link_item STREQUAL previous_link_item)
        set(previous_link_item "${unwrapped_link_item}")
        if(unwrapped_link_item MATCHES "^\\$<LINK_ONLY:([^>]+)>$")
            set(unwrapped_link_item "${CMAKE_MATCH_1}")
        elseif(unwrapped_link_item MATCHES
               "^\\$<BUILD_INTERFACE:([^>]+)>$"
        )
            set(unwrapped_link_item "${CMAKE_MATCH_1}")
        elseif(unwrapped_link_item MATCHES
               "^\\$<INSTALL_INTERFACE:([^>]+)>$"
        )
            set(unwrapped_link_item "${CMAKE_MATCH_1}")
        elseif(unwrapped_link_item MATCHES
               "^\\$<TARGET_NAME_IF_EXISTS:([^>]+)>$"
        )
            set(unwrapped_link_item "${CMAKE_MATCH_1}")
        endif()
    endwhile()

    set(${result} "${unwrapped_link_item}" PARENT_SCOPE)
endfunction()

function(
    _snodec_collect_target_component_dependencies
    target
    own_component
    dependency_kind
    result
)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "Unknown SNode.C target '${target}'")
    endif()
    if(NOT dependency_kind MATCHES "^(RUNTIME|DEVELOPMENT)$")
        message(
            FATAL_ERROR
                "Unknown SNode.C dependency kind '${dependency_kind}'"
        )
    endif()

    _snodec_real_target("${target}" real_target)
    set(pending_targets "${real_target}")
    set(visited_targets)
    set(component_dependencies)

    while(pending_targets)
        list(POP_FRONT pending_targets current_target)
        _snodec_real_target("${current_target}" current_target)
        if(current_target IN_LIST visited_targets)
            continue()
        endif()
        list(APPEND visited_targets "${current_target}")

        get_target_property(target_type "${current_target}" TYPE)
        if(dependency_kind STREQUAL "DEVELOPMENT"
           OR target_type STREQUAL "INTERFACE_LIBRARY"
        )
            set(link_property INTERFACE_LINK_LIBRARIES)
        else()
            set(link_property LINK_LIBRARIES)
        endif()
        get_target_property(
            link_libraries "${current_target}" "${link_property}"
        )

        if(NOT link_libraries
           OR link_libraries STREQUAL "link_libraries-NOTFOUND"
        )
            continue()
        endif()

        foreach(link_item IN LISTS link_libraries)
            _snodec_unwrap_link_item("${link_item}" dependency_target)
            if(NOT TARGET "${dependency_target}")
                continue()
            endif()

            _snodec_real_target("${dependency_target}" dependency_target)
            get_target_property(
                dependency_component "${dependency_target}" SNODEC_COMPONENT
            )
            if(dependency_component
               AND NOT dependency_component STREQUAL
                       "dependency_component-NOTFOUND"
            )
                if(NOT dependency_component STREQUAL own_component)
                    list(
                        APPEND component_dependencies
                        "${dependency_component}"
                    )
                    continue()
                endif()
            endif()

            get_target_property(
                dependency_imported "${dependency_target}" IMPORTED
            )
            if(dependency_imported)
                continue()
            endif()

            get_target_property(
                dependency_type "${dependency_target}" TYPE
            )
            if(NOT dependency_component
               AND dependency_type MATCHES "^(SHARED|MODULE)_LIBRARY$"
            )
                message(
                    FATAL_ERROR
                        "Target '${current_target}' links unpackaged shared target '${dependency_target}'"
                )
            endif()
            list(APPEND pending_targets "${dependency_target}")
        endforeach()
    endwhile()

    list(REMOVE_DUPLICATES component_dependencies)
    list(SORT component_dependencies)
    set(${result} "${component_dependencies}" PARENT_SCOPE)
endfunction()

function(
    _snodec_validate_component_dependency_visit
    component
    dependency_kind
    component_path
)
    _snodec_component_key("${component}" component_key)
    set(state_property
        "SNODEC_${dependency_kind}_COMPONENT_GRAPH_${component_key}_STATE"
    )
    get_property(component_state GLOBAL PROPERTY "${state_property}")

    if(component_state STREQUAL "VISITING")
        list(APPEND component_path "${component}")
        string(JOIN " -> " cycle_path ${component_path})
        string(TOLOWER "${dependency_kind}" dependency_kind_label)
        message(
            FATAL_ERROR
                "Cyclic SNode.C ${dependency_kind_label} component dependency: ${cycle_path}"
        )
    endif()
    if(component_state STREQUAL "VISITED")
        return()
    endif()

    set_property(GLOBAL PROPERTY "${state_property}" VISITING)
    list(APPEND component_path "${component}")

    _snodec_component_dependencies(
        "${component}" "${dependency_kind}" component_dependencies
    )
    get_property(registered_components GLOBAL PROPERTY SNODEC_BASE_COMPONENTS)
    foreach(dependency IN LISTS component_dependencies)
        if(NOT dependency IN_LIST registered_components)
            message(
                FATAL_ERROR
                    "SNode.C component '${component}' depends on unregistered component '${dependency}'"
            )
        endif()
        _snodec_validate_component_dependency_visit(
            "${dependency}" "${dependency_kind}" "${component_path}"
        )
    endforeach()

    set_property(GLOBAL PROPERTY "${state_property}" VISITED)
endfunction()

function(_snodec_validate_component_dependency_graph dependency_kind)
    if(NOT dependency_kind MATCHES "^(RUNTIME|DEVELOPMENT)$")
        message(
            FATAL_ERROR
                "Unknown SNode.C dependency kind '${dependency_kind}'"
        )
    endif()

    get_property(registered_components GLOBAL PROPERTY SNODEC_BASE_COMPONENTS)
    list(REMOVE_DUPLICATES registered_components)
    foreach(component IN LISTS registered_components)
        _snodec_component_key("${component}" component_key)
        set_property(
            GLOBAL
            PROPERTY
                "SNODEC_${dependency_kind}_COMPONENT_GRAPH_${component_key}_STATE"
                ""
        )
    endforeach()

    foreach(component IN LISTS registered_components)
        _snodec_validate_component_dependency_visit(
            "${component}" "${dependency_kind}" ""
        )
    endforeach()
endfunction()

function(_snodec_component_dependencies component dependency_kind result)
    _snodec_component_property("${component}" TARGETS component_targets)

    if(dependency_kind STREQUAL "RUNTIME")
        _snodec_component_property(
            "${component}"
            EXPLICIT_RUNTIME_DEPENDS
            component_dependencies
        )
    elseif(dependency_kind STREQUAL "DEVELOPMENT")
        _snodec_component_property(
            "${component}"
            EXPLICIT_DEVELOPMENT_DEPENDS
            component_dependencies
        )
    else()
        message(
            FATAL_ERROR
                "Unknown SNode.C dependency kind '${dependency_kind}'"
        )
    endif()

    foreach(target IN LISTS component_targets)
        _snodec_collect_target_component_dependencies(
            "${target}"
            "${component}"
            "${dependency_kind}"
            target_component_dependencies
        )
        list(APPEND component_dependencies ${target_component_dependencies})
    endforeach()

    list(REMOVE_ITEM component_dependencies "${component}")
    list(REMOVE_DUPLICATES component_dependencies)
    list(SORT component_dependencies)
    set(${result} "${component_dependencies}" PARENT_SCOPE)
endfunction()

function(snodec_add_component)
    set(options PUBLIC_COMPONENT)
    set(
        one_value_arguments
        TARGET
        COMPONENT
        RUNTIME_DESTINATION
        LIBRARY_DESTINATION
        ARCHIVE_DESTINATION
        EXPORT_DESTINATION
        DISPLAY_NAME
        DESCRIPTION
    )
    set(multi_value_arguments DEPENDS DEVELOPMENT_DEPENDS)
    cmake_parse_arguments(
        ARG "${options}" "${one_value_arguments}" "${multi_value_arguments}"
        ${ARGN}
    )
    _snodec_validate_arguments(ARG "snodec_add_component")

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "TARGET is required")
    endif()
    if(NOT TARGET "${ARG_TARGET}")
        message(FATAL_ERROR "Unknown SNode.C target '${ARG_TARGET}'")
    endif()

    _snodec_real_target("${ARG_TARGET}" real_target)
    get_target_property(target_type "${real_target}" TYPE)
    if(NOT target_type STREQUAL "SHARED_LIBRARY")
        message(
            FATAL_ERROR
                "snodec_add_component expects a shared library, got '${ARG_TARGET}' (${target_type})"
        )
    endif()
    _snodec_record_shlibdeps_private_directory("${real_target}")

    if(NOT ARG_COMPONENT)
        set(ARG_COMPONENT "${ARG_TARGET}")
    endif()
    if(NOT ARG_RUNTIME_DESTINATION)
        set(ARG_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
    if(NOT ARG_LIBRARY_DESTINATION)
        set(ARG_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    endif()
    if(NOT ARG_ARCHIVE_DESTINATION)
        set(ARG_ARCHIVE_DESTINATION "${ARG_LIBRARY_DESTINATION}")
    endif()
    if(NOT ARG_EXPORT_DESTINATION)
        set(ARG_EXPORT_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/snodec")
    endif()

    set(
        register_arguments
        COMPONENT "${ARG_COMPONENT}"
        TARGETS "${real_target}"
    )
    if(ARG_DISPLAY_NAME)
        list(APPEND register_arguments DISPLAY_NAME "${ARG_DISPLAY_NAME}")
    endif()
    if(ARG_DESCRIPTION)
        list(APPEND register_arguments DESCRIPTION "${ARG_DESCRIPTION}")
    endif()
    if(ARG_DEPENDS)
        list(APPEND register_arguments DEPENDS ${ARG_DEPENDS})
    endif()
    if(ARG_DEVELOPMENT_DEPENDS)
        list(
            APPEND register_arguments DEVELOPMENT_DEPENDS
            ${ARG_DEVELOPMENT_DEPENDS}
        )
    endif()
    if(ARG_PUBLIC_COMPONENT)
        list(APPEND register_arguments PUBLIC)
    endif()
    _snodec_register_component(${register_arguments})

    set(development_component "${ARG_COMPONENT}-dev")
    set(export_set "snodec_${ARG_COMPONENT}_Targets")
    _snodec_record_install_component("${ARG_COMPONENT}")
    _snodec_record_install_component("${development_component}")

    install(
        TARGETS "${real_target}"
        EXPORT "${export_set}"
        RUNTIME DESTINATION "${ARG_RUNTIME_DESTINATION}"
                COMPONENT "${ARG_COMPONENT}"
        LIBRARY DESTINATION "${ARG_LIBRARY_DESTINATION}"
                COMPONENT "${ARG_COMPONENT}"
                NAMELINK_COMPONENT "${development_component}"
        ARCHIVE DESTINATION "${ARG_ARCHIVE_DESTINATION}"
                COMPONENT "${development_component}"
        BUNDLE DESTINATION "${ARG_RUNTIME_DESTINATION}"
               COMPONENT "${ARG_COMPONENT}"
    )

    install(
        EXPORT "${export_set}"
        FILE "${export_set}.cmake"
        NAMESPACE snodec::
        DESTINATION "${ARG_EXPORT_DESTINATION}"
        COMPONENT "${development_component}"
    )
endfunction()

function(snodec_install_component_headers)
    set(options)
    set(one_value_arguments COMPONENT DIRECTORY DESTINATION)
    set(multi_value_arguments FILES PATTERNS EXCLUDE_PATTERNS)
    cmake_parse_arguments(
        ARG "${options}" "${one_value_arguments}" "${multi_value_arguments}"
        ${ARGN}
    )
    _snodec_validate_arguments(ARG "snodec_install_component_headers")

    if(NOT ARG_COMPONENT)
        message(FATAL_ERROR "COMPONENT is required")
    endif()
    if(ARG_COMPONENT MATCHES "-dev$")
        message(
            FATAL_ERROR
                "Use the base component name, not '${ARG_COMPONENT}', for development files"
        )
    endif()
    if(NOT ARG_DESTINATION)
        message(
            FATAL_ERROR
                "DESTINATION is required for component '${ARG_COMPONENT}'"
        )
    endif()
    if(ARG_DIRECTORY AND ARG_FILES)
        message(
            FATAL_ERROR
                "Use either DIRECTORY or FILES for component '${ARG_COMPONENT}'"
        )
    endif()
    if(NOT ARG_DIRECTORY AND NOT ARG_FILES)
        message(
            FATAL_ERROR
                "DIRECTORY or FILES is required for component '${ARG_COMPONENT}'"
        )
    endif()

    set(development_component "${ARG_COMPONENT}-dev")
    _snodec_record_install_component("${development_component}")

    if(ARG_FILES)
        install(
            FILES ${ARG_FILES}
            DESTINATION "${ARG_DESTINATION}"
            COMPONENT "${development_component}"
        )
        return()
    endif()

    set(source_directory "${ARG_DIRECTORY}")
    string(REGEX REPLACE "/+$" "" source_directory "${source_directory}")

    set(pattern_arguments)
    if(ARG_PATTERNS OR ARG_EXCLUDE_PATTERNS)
        list(APPEND pattern_arguments FILES_MATCHING)
    endif()
    foreach(pattern IN LISTS ARG_PATTERNS)
        list(APPEND pattern_arguments PATTERN "${pattern}")
    endforeach()
    foreach(pattern IN LISTS ARG_EXCLUDE_PATTERNS)
        list(APPEND pattern_arguments PATTERN "${pattern}" EXCLUDE)
    endforeach()

    install(
        DIRECTORY "${source_directory}/"
        DESTINATION "${ARG_DESTINATION}"
        COMPONENT "${development_component}"
        ${pattern_arguments}
    )
endfunction()

function(snodec_install_component_development_files)
    set(options)
    set(one_value_arguments COMPONENT DESTINATION)
    set(multi_value_arguments FILES)
    cmake_parse_arguments(
        ARG "${options}" "${one_value_arguments}" "${multi_value_arguments}"
        ${ARGN}
    )
    _snodec_validate_arguments(
        ARG "snodec_install_component_development_files"
    )

    if(NOT ARG_COMPONENT)
        message(FATAL_ERROR "COMPONENT is required")
    endif()
    if(NOT ARG_FILES)
        message(FATAL_ERROR "FILES is required")
    endif()

    snodec_install_component_headers(
        COMPONENT "${ARG_COMPONENT}"
        FILES ${ARG_FILES}
        DESTINATION "${ARG_DESTINATION}"
    )
endfunction()

function(snodec_install_runtime_targets)
    set(options)
    set(one_value_arguments COMPONENT RUNTIME_DESTINATION LIBRARY_DESTINATION)
    set(multi_value_arguments TARGETS)
    cmake_parse_arguments(
        ARG "${options}" "${one_value_arguments}" "${multi_value_arguments}"
        ${ARGN}
    )
    _snodec_validate_arguments(ARG "snodec_install_runtime_targets")

    if(NOT ARG_COMPONENT)
        message(FATAL_ERROR "COMPONENT is required")
    endif()
    if(NOT ARG_TARGETS)
        message(FATAL_ERROR "TARGETS is required")
    endif()
    if(NOT ARG_RUNTIME_DESTINATION)
        set(ARG_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
    if(NOT ARG_LIBRARY_DESTINATION)
        set(ARG_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    endif()

    foreach(target IN LISTS ARG_TARGETS)
        if(NOT TARGET "${target}")
            message(FATAL_ERROR "Unknown SNode.C target '${target}'")
        endif()
        _snodec_real_target("${target}" real_target)
        _snodec_set_target_component("${real_target}" "${ARG_COMPONENT}")
        _snodec_record_shlibdeps_private_directory("${real_target}")
        _snodec_append_component_property(
            "${ARG_COMPONENT}" PENDING_TARGETS "${real_target}"
        )
    endforeach()

    _snodec_record_install_component("${ARG_COMPONENT}")
    install(
        TARGETS ${ARG_TARGETS}
        RUNTIME DESTINATION "${ARG_RUNTIME_DESTINATION}"
                COMPONENT "${ARG_COMPONENT}"
        LIBRARY DESTINATION "${ARG_LIBRARY_DESTINATION}"
                COMPONENT "${ARG_COMPONENT}"
        BUNDLE DESTINATION "${ARG_RUNTIME_DESTINATION}"
               COMPONENT "${ARG_COMPONENT}"
    )
endfunction()

function(snodec_register_runtime_component)
    set(options)
    set(one_value_arguments COMPONENT DISPLAY_NAME DESCRIPTION)
    set(multi_value_arguments DEPENDS)
    cmake_parse_arguments(
        ARG "${options}" "${one_value_arguments}" "${multi_value_arguments}"
        ${ARGN}
    )
    _snodec_validate_arguments(ARG "snodec_register_runtime_component")

    if(NOT ARG_COMPONENT)
        message(FATAL_ERROR "COMPONENT is required")
    endif()

    _snodec_component_property(
        "${ARG_COMPONENT}" PENDING_TARGETS component_targets
    )
    if(NOT component_targets)
        message(
            FATAL_ERROR
                "Runtime component '${ARG_COMPONENT}' has no installed targets"
        )
    endif()

    set(
        register_arguments
        COMPONENT "${ARG_COMPONENT}"
        TARGETS ${component_targets}
        RUNTIME_ONLY
    )
    if(ARG_DISPLAY_NAME)
        list(APPEND register_arguments DISPLAY_NAME "${ARG_DISPLAY_NAME}")
    endif()
    if(ARG_DESCRIPTION)
        list(APPEND register_arguments DESCRIPTION "${ARG_DESCRIPTION}")
    endif()
    if(ARG_DEPENDS)
        list(APPEND register_arguments DEPENDS ${ARG_DEPENDS})
    endif()
    _snodec_register_component(${register_arguments})
endfunction()

function(snodec_get_public_components result)
    get_property(public_components GLOBAL PROPERTY SNODEC_PUBLIC_COMPONENTS)
    list(REMOVE_DUPLICATES public_components)
    list(SORT public_components)
    set(${result} "${public_components}" PARENT_SCOPE)
endfunction()

function(snodec_get_component_dependency_definitions result)
    get_property(components GLOBAL PROPERTY SNODEC_BASE_COMPONENTS)
    list(REMOVE_DUPLICATES components)
    list(SORT components)

    set(dependency_definitions)
    foreach(component IN LISTS components)
        _snodec_component_property(
            "${component}" RUNTIME_ONLY runtime_only
        )
        if(runtime_only)
            continue()
        endif()

        _snodec_component_dependencies(
            "${component}" DEVELOPMENT dependencies
        )
        if(dependencies)
            string(
                APPEND dependency_definitions
                "set(${component}_DEPENDENCIES ${dependencies})\n"
            )
        endif()
    endforeach()

    string(
        REGEX REPLACE "\n$" "" dependency_definitions
        "${dependency_definitions}"
    )
    set(${result} "${dependency_definitions}" PARENT_SCOPE)
endfunction()

macro(snodec_finalize_cpack_components)
    get_property(_snodec_base_components GLOBAL PROPERTY SNODEC_BASE_COMPONENTS)
    get_property(
        _snodec_referenced_install_components GLOBAL
        PROPERTY SNODEC_REFERENCED_INSTALL_COMPONENTS
    )
    list(REMOVE_DUPLICATES _snodec_base_components)
    list(REMOVE_DUPLICATES _snodec_referenced_install_components)

    if(NOT _snodec_base_components)
        message(FATAL_ERROR "No SNode.C components were registered")
    endif()

    _snodec_validate_component_dependency_graph(RUNTIME)
    _snodec_validate_component_dependency_graph(DEVELOPMENT)

    set(_snodec_cpack_components)
    foreach(_snodec_component IN LISTS _snodec_base_components)
        _snodec_component_property(
            "${_snodec_component}" DISPLAY_NAME _snodec_display_name
        )
        _snodec_component_property(
            "${_snodec_component}" DESCRIPTION _snodec_description
        )
        _snodec_component_property(
            "${_snodec_component}" RUNTIME_ONLY _snodec_runtime_only
        )
        _snodec_component_dependencies(
            "${_snodec_component}"
            RUNTIME
            _snodec_runtime_dependencies
        )
        _snodec_component_dependencies(
            "${_snodec_component}"
            DEVELOPMENT
            _snodec_development_dependencies
        )

        set(
            _snodec_referenced_dependencies
            ${_snodec_runtime_dependencies}
            ${_snodec_development_dependencies}
        )
        list(REMOVE_DUPLICATES _snodec_referenced_dependencies)
        foreach(_snodec_dependency IN LISTS _snodec_referenced_dependencies)
            if(NOT _snodec_dependency IN_LIST _snodec_base_components)
                message(
                    FATAL_ERROR
                        "SNode.C component '${_snodec_component}' depends on unregistered component '${_snodec_dependency}'"
                )
            endif()
        endforeach()

        if(NOT _snodec_component IN_LIST _snodec_referenced_install_components)
            message(
                FATAL_ERROR
                    "SNode.C component '${_snodec_component}' has no runtime install rules"
            )
        endif()

        set(_snodec_runtime_arguments)
        if(_snodec_display_name)
            list(
                APPEND _snodec_runtime_arguments DISPLAY_NAME
                "${_snodec_display_name}"
            )
        endif()
        if(_snodec_description)
            list(
                APPEND _snodec_runtime_arguments DESCRIPTION
                "${_snodec_description}"
            )
        endif()
        if(_snodec_runtime_dependencies)
            list(
                APPEND _snodec_runtime_arguments DEPENDS
                ${_snodec_runtime_dependencies}
            )
        endif()
        cpack_add_component(
            "${_snodec_component}" ${_snodec_runtime_arguments}
        )
        list(APPEND _snodec_cpack_components "${_snodec_component}")

        if(NOT _snodec_runtime_only)
            set(_snodec_development_component "${_snodec_component}-dev")
            if(NOT _snodec_development_component IN_LIST
                   _snodec_referenced_install_components
            )
                message(
                    FATAL_ERROR
                        "SNode.C component '${_snodec_component}' has no development install rules"
                )
            endif()

            set(
                _snodec_cpack_development_dependencies
                "${_snodec_component}"
            )
            foreach(_snodec_dependency IN LISTS
                    _snodec_development_dependencies
            )
                _snodec_component_property(
                    "${_snodec_dependency}"
                    RUNTIME_ONLY
                    _snodec_dependency_runtime_only
                )
                if(_snodec_dependency_runtime_only)
                    list(
                        APPEND _snodec_cpack_development_dependencies
                        "${_snodec_dependency}"
                    )
                else()
                    list(
                        APPEND _snodec_cpack_development_dependencies
                        "${_snodec_dependency}-dev"
                    )
                endif()
            endforeach()
            list(REMOVE_DUPLICATES _snodec_cpack_development_dependencies)
            list(SORT _snodec_cpack_development_dependencies)

            cpack_add_component(
                "${_snodec_development_component}"
                DISPLAY_NAME "${_snodec_component} development"
                DESCRIPTION
                    "Headers and CMake package metadata for ${_snodec_component}"
                DEPENDS ${_snodec_cpack_development_dependencies}
            )
            list(
                APPEND _snodec_cpack_components
                "${_snodec_development_component}"
            )
        endif()
    endforeach()

    foreach(_snodec_referenced_component IN LISTS
            _snodec_referenced_install_components
    )
        if(NOT _snodec_referenced_component IN_LIST _snodec_cpack_components)
            message(
                FATAL_ERROR
                    "Install rules reference unregistered SNode.C component '${_snodec_referenced_component}'"
            )
        endif()
    endforeach()

    get_cmake_property(_snodec_cmake_install_components COMPONENTS)
    list(REMOVE_ITEM _snodec_cmake_install_components notneeded)
    list(REMOVE_DUPLICATES _snodec_cmake_install_components)
    foreach(_snodec_install_component IN LISTS
            _snodec_cmake_install_components
    )
        if(NOT _snodec_install_component IN_LIST _snodec_cpack_components)
            message(
                FATAL_ERROR
                    "CMake install component '${_snodec_install_component}' has no SNode.C CPack registration"
            )
        endif()
    endforeach()

    get_property(
        _snodec_shlibdeps_private_dirs GLOBAL
        PROPERTY SNODEC_SHLIBDEPS_PRIVATE_DIRS
    )
    list(REMOVE_DUPLICATES _snodec_shlibdeps_private_dirs)
    list(SORT _snodec_shlibdeps_private_dirs)

    list(REMOVE_DUPLICATES _snodec_cpack_components)
    list(SORT _snodec_cpack_components)
    set(CPACK_COMPONENTS_ALL ${_snodec_cpack_components})
    set(
        CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS
        ${_snodec_shlibdeps_private_dirs}
    )
endmacro()
