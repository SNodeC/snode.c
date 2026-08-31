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
include(CMakePackageConfigHelpers)
include(CPackComponent)
include(GNUInstallDirs)

# Every component is declared where its target is declared. The registry is
# package-format neutral: generic finalization owns install/export generation
# and resolved dependencies, while adapters such as CPack -- and a future
# OpenWrt package generator -- consume the same finalized metadata.

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
        "${ARG_COMPONENT}" PUBLIC "${ARG_PUBLIC}"
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

function(_snodec_require_not_finalized command_name)
    get_property(
        components_finalized GLOBAL PROPERTY SNODEC_COMPONENTS_FINALIZED
    )
    if(components_finalized)
        message(
            FATAL_ERROR
                "${command_name} cannot modify the SNode.C component registry "
                "after snodec_finalize_components()"
        )
    endif()
endfunction()

function(_snodec_require_finalized command_name)
    get_property(
        components_finalized GLOBAL PROPERTY SNODEC_COMPONENTS_FINALIZED
    )
    if(NOT components_finalized)
        message(
            FATAL_ERROR
                "${command_name} requires snodec_finalize_components() first"
        )
    endif()
endfunction()

function(_snodec_absolute_source_path source_path result)
    if(IS_ABSOLUTE "${source_path}")
        set(absolute_path "${source_path}")
    else()
        get_filename_component(
            absolute_path "${source_path}" ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
    set(${result} "${absolute_path}" PARENT_SCOPE)
endfunction()

function(_snodec_record_development_set component set_kind set_index)
    set(options)
    set(one_value_arguments DIRECTORY DESTINATION)
    set(multi_value_arguments FILES PATTERNS EXCLUDE_PATTERNS)
    cmake_parse_arguments(
        SET "${options}" "${one_value_arguments}" "${multi_value_arguments}"
        ${ARGN}
    )
    _snodec_validate_arguments(SET "snodec_add_component ${set_kind}")

    if(NOT SET_DESTINATION)
        message(
            FATAL_ERROR
                "${set_kind} DESTINATION is required for component '${component}'"
        )
    endif()
    if(SET_DIRECTORY AND SET_FILES)
        message(
            FATAL_ERROR
                "${set_kind} for component '${component}' must use either DIRECTORY or FILES"
        )
    endif()
    if(NOT SET_DIRECTORY AND NOT SET_FILES)
        message(
            FATAL_ERROR
                "${set_kind} for component '${component}' requires DIRECTORY or FILES"
        )
    endif()
    if(SET_FILES AND (SET_PATTERNS OR SET_EXCLUDE_PATTERNS))
        message(
            FATAL_ERROR
                "${set_kind} PATTERNS are only valid with DIRECTORY for component '${component}'"
        )
    endif()

    if(SET_DIRECTORY)
        _snodec_absolute_source_path("${SET_DIRECTORY}" source_directory)
        string(REGEX REPLACE "/+$" "" source_directory "${source_directory}")
    endif()

    set(source_files)
    foreach(source_file IN LISTS SET_FILES)
        _snodec_absolute_source_path("${source_file}" absolute_source_file)
        list(APPEND source_files "${absolute_source_file}")
    endforeach()

    _snodec_set_component_property(
        "${component}" "${set_kind}_${set_index}_DIRECTORY"
        "${source_directory}"
    )
    _snodec_set_component_property(
        "${component}" "${set_kind}_${set_index}_FILES" ${source_files}
    )
    _snodec_set_component_property(
        "${component}" "${set_kind}_${set_index}_DESTINATION"
        "${SET_DESTINATION}"
    )
    _snodec_set_component_property(
        "${component}" "${set_kind}_${set_index}_PATTERNS" ${SET_PATTERNS}
    )
    _snodec_set_component_property(
        "${component}" "${set_kind}_${set_index}_EXCLUDE_PATTERNS"
        ${SET_EXCLUDE_PATTERNS}
    )
endfunction()

function(_snodec_append_development_files component destination)
    set(source_files)
    foreach(source_file IN LISTS ARGN)
        _snodec_absolute_source_path("${source_file}" absolute_source_file)
        list(APPEND source_files "${absolute_source_file}")
    endforeach()

    _snodec_component_property(
        "${component}" DEVELOPMENT_FILES_COUNT development_files_count
    )
    if(NOT development_files_count)
        set(development_files_count 0)
    endif()
    math(EXPR development_files_count "${development_files_count} + 1")
    _snodec_set_component_property(
        "${component}" DEVELOPMENT_FILES_COUNT "${development_files_count}"
    )
    _snodec_record_development_set(
        "${component}" DEVELOPMENT_FILES "${development_files_count}"
        FILES ${source_files}
        DESTINATION "${destination}"
    )
endfunction()

# Declare one shared-library component, including all public headers and other
# development artifacts. HEADERS and DEVELOPMENT_FILES are repeatable sections;
# each section accepts FILES or DIRECTORY, DESTINATION, and (for DIRECTORY)
# PATTERNS/EXCLUDE_PATTERNS. No install rules are emitted until finalization.
function(snodec_add_component)
    _snodec_require_not_finalized("snodec_add_component")

    # Core component metadata precedes optional repeated HEADERS and
    # DEVELOPMENT_FILES sections. Keeping all installable development content
    # in this declaration gives CPack, CMake package exports, and future
    # packaging adapters (notably OpenWrt) one canonical component model.
    set(core_arguments)
    set(current_section CORE)
    set(header_set_count 0)
    set(development_files_count 0)
    foreach(argument IN LISTS ARGN)
        if(argument STREQUAL "HEADERS")
            set(current_section HEADERS)
            math(EXPR header_set_count "${header_set_count} + 1")
        elseif(argument STREQUAL "DEVELOPMENT_FILES")
            set(current_section DEVELOPMENT_FILES)
            math(EXPR development_files_count "${development_files_count} + 1")
        elseif(current_section STREQUAL "CORE")
            list(APPEND core_arguments "${argument}")
        elseif(current_section STREQUAL "HEADERS")
            list(
                APPEND header_set_${header_set_count}_arguments "${argument}"
            )
        else()
            list(
                APPEND development_files_${development_files_count}_arguments
                "${argument}"
            )
        endif()
    endforeach()

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
        ${core_arguments}
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

    _snodec_set_component_property(
        "${ARG_COMPONENT}" DECLARATION_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" RUNTIME_DESTINATION "${ARG_RUNTIME_DESTINATION}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" LIBRARY_DESTINATION "${ARG_LIBRARY_DESTINATION}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" ARCHIVE_DESTINATION "${ARG_ARCHIVE_DESTINATION}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" EXPORT_DESTINATION "${ARG_EXPORT_DESTINATION}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" EXPORT_SET "snodec_${ARG_COMPONENT}_Targets"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" HEADER_SET_COUNT "${header_set_count}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}" DEVELOPMENT_FILES_COUNT
        "${development_files_count}"
    )

    if(header_set_count GREATER 0)
        foreach(header_set_index RANGE 1 ${header_set_count})
            _snodec_record_development_set(
                "${ARG_COMPONENT}" HEADERS "${header_set_index}"
                ${header_set_${header_set_index}_arguments}
            )
        endforeach()
    endif()
    if(development_files_count GREATER 0)
        foreach(development_files_index RANGE 1 ${development_files_count})
            _snodec_record_development_set(
                "${ARG_COMPONENT}" DEVELOPMENT_FILES
                "${development_files_index}"
                ${development_files_${development_files_index}_arguments}
            )
        endforeach()
    endif()
endfunction()

function(snodec_install_runtime_targets)
    _snodec_require_not_finalized("snodec_install_runtime_targets")

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

    set(real_targets)
    foreach(target IN LISTS ARG_TARGETS)
        if(NOT TARGET "${target}")
            message(FATAL_ERROR "Unknown SNode.C target '${target}'")
        endif()
        _snodec_real_target("${target}" real_target)
        _snodec_set_target_component("${real_target}" "${ARG_COMPONENT}")
        _snodec_record_shlibdeps_private_directory("${real_target}")
        list(APPEND real_targets "${real_target}")
        _snodec_append_component_property(
            "${ARG_COMPONENT}" PENDING_TARGETS "${real_target}"
        )
    endforeach()

    _snodec_component_property(
        "${ARG_COMPONENT}" RUNTIME_TARGET_SET_COUNT runtime_target_set_count
    )
    if(NOT runtime_target_set_count)
        set(runtime_target_set_count 0)
    endif()
    math(EXPR runtime_target_set_count "${runtime_target_set_count} + 1")
    _snodec_set_component_property(
        "${ARG_COMPONENT}" RUNTIME_TARGET_SET_COUNT
        "${runtime_target_set_count}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}"
        "RUNTIME_TARGET_SET_${runtime_target_set_count}_TARGETS"
        ${real_targets}
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}"
        "RUNTIME_TARGET_SET_${runtime_target_set_count}_RUNTIME_DESTINATION"
        "${ARG_RUNTIME_DESTINATION}"
    )
    _snodec_set_component_property(
        "${ARG_COMPONENT}"
        "RUNTIME_TARGET_SET_${runtime_target_set_count}_LIBRARY_DESTINATION"
        "${ARG_LIBRARY_DESTINATION}"
    )
    set_property(
        GLOBAL APPEND PROPERTY SNODEC_PENDING_RUNTIME_COMPONENTS
        "${ARG_COMPONENT}"
    )
endfunction()

function(snodec_register_runtime_component)
    _snodec_require_not_finalized("snodec_register_runtime_component")

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
                "Runtime component '${ARG_COMPONENT}' has no registered targets"
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

function(_snodec_registered_components result)
    get_property(components GLOBAL PROPERTY SNODEC_BASE_COMPONENTS)
    list(REMOVE_DUPLICATES components)
    list(SORT components)
    set(${result} "${components}" PARENT_SCOPE)
endfunction()

function(_snodec_public_components result)
    get_property(public_components GLOBAL PROPERTY SNODEC_PUBLIC_COMPONENTS)
    list(REMOVE_DUPLICATES public_components)
    list(SORT public_components)
    set(${result} "${public_components}" PARENT_SCOPE)
endfunction()

function(_snodec_component_dependency_definitions result)
    _snodec_registered_components(components)

    set(dependency_definitions)
    foreach(component IN LISTS components)
        _snodec_component_property(
            "${component}" RUNTIME_ONLY runtime_only
        )
        if(runtime_only)
            continue()
        endif()

        _snodec_component_property(
            "${component}" RESOLVED_DEVELOPMENT_DEPENDS dependencies
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

function(_snodec_materialize_development_set component set_kind set_index)
    _snodec_component_property(
        "${component}" "${set_kind}_${set_index}_DIRECTORY" source_directory
    )
    _snodec_component_property(
        "${component}" "${set_kind}_${set_index}_FILES" source_files
    )
    _snodec_component_property(
        "${component}" "${set_kind}_${set_index}_DESTINATION" destination
    )
    _snodec_component_property(
        "${component}" "${set_kind}_${set_index}_PATTERNS" patterns
    )
    _snodec_component_property(
        "${component}" "${set_kind}_${set_index}_EXCLUDE_PATTERNS"
        exclude_patterns
    )

    set(development_component "${component}-dev")
    _snodec_record_install_component("${development_component}")

    if(source_files)
        install(
            FILES ${source_files}
            DESTINATION "${destination}"
            COMPONENT "${development_component}"
        )
        return()
    endif()

    set(pattern_arguments)
    if(patterns OR exclude_patterns)
        list(APPEND pattern_arguments FILES_MATCHING)
    endif()
    foreach(pattern IN LISTS patterns)
        list(APPEND pattern_arguments PATTERN "${pattern}")
    endforeach()
    foreach(pattern IN LISTS exclude_patterns)
        list(APPEND pattern_arguments PATTERN "${pattern}" EXCLUDE)
    endforeach()

    install(
        DIRECTORY "${source_directory}/"
        DESTINATION "${destination}"
        COMPONENT "${development_component}"
        ${pattern_arguments}
    )
endfunction()

function(_snodec_materialize_library_component component)
    _snodec_component_property("${component}" TARGETS component_targets)
    list(LENGTH component_targets component_target_count)
    if(NOT component_target_count EQUAL 1)
        message(
            FATAL_ERROR
                "Library component '${component}' must own exactly one target"
        )
    endif()
    list(GET component_targets 0 component_target)

    _snodec_component_property(
        "${component}" RUNTIME_DESTINATION runtime_destination
    )
    _snodec_component_property(
        "${component}" LIBRARY_DESTINATION library_destination
    )
    _snodec_component_property(
        "${component}" ARCHIVE_DESTINATION archive_destination
    )
    _snodec_component_property(
        "${component}" EXPORT_DESTINATION export_destination
    )
    _snodec_component_property("${component}" EXPORT_SET export_set)

    set(development_component "${component}-dev")
    _snodec_record_install_component("${component}")
    _snodec_record_install_component("${development_component}")

    install(
        TARGETS "${component_target}"
        EXPORT "${export_set}"
        RUNTIME DESTINATION "${runtime_destination}"
                COMPONENT "${component}"
        LIBRARY DESTINATION "${library_destination}"
                COMPONENT "${component}"
                NAMELINK_COMPONENT "${development_component}"
        ARCHIVE DESTINATION "${archive_destination}"
                COMPONENT "${development_component}"
        BUNDLE DESTINATION "${runtime_destination}"
               COMPONENT "${component}"
    )
    install(
        EXPORT "${export_set}"
        FILE "${export_set}.cmake"
        NAMESPACE snodec::
        DESTINATION "${export_destination}"
        COMPONENT "${development_component}"
    )

    _snodec_component_property(
        "${component}" HEADER_SET_COUNT header_set_count
    )
    if(header_set_count GREATER 0)
        foreach(header_set_index RANGE 1 ${header_set_count})
            _snodec_materialize_development_set(
                "${component}" HEADERS "${header_set_index}"
            )
        endforeach()
    endif()

    _snodec_component_property(
        "${component}" DEVELOPMENT_FILES_COUNT development_files_count
    )
    if(development_files_count GREATER 0)
        foreach(development_files_index RANGE 1 ${development_files_count})
            _snodec_materialize_development_set(
                "${component}" DEVELOPMENT_FILES
                "${development_files_index}"
            )
        endforeach()
    endif()
endfunction()

function(_snodec_materialize_runtime_component component)
    _snodec_component_property(
        "${component}" RUNTIME_TARGET_SET_COUNT runtime_target_set_count
    )
    if(NOT runtime_target_set_count
       OR NOT runtime_target_set_count GREATER 0
    )
        message(
            FATAL_ERROR
                "Runtime component '${component}' has no target install sets"
        )
    endif()

    _snodec_record_install_component("${component}")
    foreach(runtime_target_set_index RANGE 1 ${runtime_target_set_count})
        _snodec_component_property(
            "${component}"
            "RUNTIME_TARGET_SET_${runtime_target_set_index}_TARGETS"
            targets
        )
        _snodec_component_property(
            "${component}"
            "RUNTIME_TARGET_SET_${runtime_target_set_index}_RUNTIME_DESTINATION"
            runtime_destination
        )
        _snodec_component_property(
            "${component}"
            "RUNTIME_TARGET_SET_${runtime_target_set_index}_LIBRARY_DESTINATION"
            library_destination
        )
        install(
            TARGETS ${targets}
            RUNTIME DESTINATION "${runtime_destination}"
                    COMPONENT "${component}"
            LIBRARY DESTINATION "${library_destination}"
                    COMPONENT "${component}"
            BUNDLE DESTINATION "${runtime_destination}"
                   COMPONENT "${component}"
        )
    endforeach()
endfunction()

# Complete the package-format-neutral registry exactly once after all component
# subdirectories have been processed. This resolves dependencies, optionally
# generates the installed CMake package, and materializes all install rules.
function(snodec_finalize_components)
    _snodec_require_not_finalized("snodec_finalize_components")

    set(options)
    set(
        one_value_arguments
        PACKAGE_CONFIG_TEMPLATE
        PACKAGE_CONFIG_OUTPUT
        PACKAGE_VERSION_OUTPUT
        PACKAGE_CONFIG_DESTINATION
        PACKAGE_CONFIG_COMPONENT
        PACKAGE_VERSION
    )
    cmake_parse_arguments(
        ARG "${options}" "${one_value_arguments}" "" ${ARGN}
    )
    _snodec_validate_arguments(ARG "snodec_finalize_components")

    _snodec_registered_components(components)
    if(NOT components)
        message(FATAL_ERROR "No SNode.C components were registered")
    endif()

    get_property(
        pending_runtime_components GLOBAL PROPERTY SNODEC_PENDING_RUNTIME_COMPONENTS
    )
    list(REMOVE_DUPLICATES pending_runtime_components)
    foreach(pending_component IN LISTS pending_runtime_components)
        if(NOT pending_component IN_LIST components)
            message(
                FATAL_ERROR
                    "Runtime targets were recorded for unregistered component '${pending_component}'"
            )
        endif()
    endforeach()

    _snodec_validate_component_dependency_graph(RUNTIME)
    _snodec_validate_component_dependency_graph(DEVELOPMENT)

    foreach(component IN LISTS components)
        _snodec_component_dependencies(
            "${component}" RUNTIME runtime_dependencies
        )
        _snodec_component_dependencies(
            "${component}" DEVELOPMENT development_dependencies
        )
        _snodec_set_component_property(
            "${component}" RESOLVED_RUNTIME_DEPENDS ${runtime_dependencies}
        )
        _snodec_set_component_property(
            "${component}" RESOLVED_DEVELOPMENT_DEPENDS
            ${development_dependencies}
        )
    endforeach()

    set(package_config_arguments_present FALSE)
    foreach(package_config_argument IN ITEMS
            ARG_PACKAGE_CONFIG_TEMPLATE
            ARG_PACKAGE_CONFIG_OUTPUT
            ARG_PACKAGE_VERSION_OUTPUT
            ARG_PACKAGE_CONFIG_DESTINATION
            ARG_PACKAGE_CONFIG_COMPONENT
            ARG_PACKAGE_VERSION
    )
        if(NOT "${${package_config_argument}}" STREQUAL "")
            set(package_config_arguments_present TRUE)
        endif()
    endforeach()

    if(package_config_arguments_present)
        foreach(required_argument IN ITEMS
                PACKAGE_CONFIG_TEMPLATE
                PACKAGE_CONFIG_OUTPUT
                PACKAGE_VERSION_OUTPUT
                PACKAGE_CONFIG_DESTINATION
                PACKAGE_CONFIG_COMPONENT
                PACKAGE_VERSION
        )
            if(NOT ARG_${required_argument})
                message(
                    FATAL_ERROR
                        "snodec_finalize_components requires ${required_argument} "
                        "when generating package configuration"
                )
            endif()
        endforeach()
        if(NOT ARG_PACKAGE_CONFIG_COMPONENT IN_LIST components)
            message(
                FATAL_ERROR
                    "Package configuration component "
                    "'${ARG_PACKAGE_CONFIG_COMPONENT}' is not registered"
            )
        endif()
        _snodec_component_property(
            "${ARG_PACKAGE_CONFIG_COMPONENT}" RUNTIME_ONLY config_runtime_only
        )
        if(config_runtime_only)
            message(
                FATAL_ERROR
                    "Package configuration cannot belong to runtime-only "
                    "component '${ARG_PACKAGE_CONFIG_COMPONENT}'"
            )
        endif()

        _snodec_public_components(public_components)
        string(JOIN "\n    " SUPPORTED_COMPONENTS ${public_components})
        _snodec_component_dependency_definitions(
            SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES
        )

        configure_package_config_file(
            "${ARG_PACKAGE_CONFIG_TEMPLATE}"
            "${ARG_PACKAGE_CONFIG_OUTPUT}"
            INSTALL_DESTINATION "${ARG_PACKAGE_CONFIG_DESTINATION}"
            NO_SET_AND_CHECK_MACRO
        )
        write_basic_package_version_file(
            "${ARG_PACKAGE_VERSION_OUTPUT}"
            VERSION "${ARG_PACKAGE_VERSION}"
            COMPATIBILITY SameMinorVersion
        )
        _snodec_append_development_files(
            "${ARG_PACKAGE_CONFIG_COMPONENT}"
            "${ARG_PACKAGE_CONFIG_DESTINATION}"
            "${ARG_PACKAGE_CONFIG_OUTPUT}"
            "${ARG_PACKAGE_VERSION_OUTPUT}"
        )
    endif()

    foreach(component IN LISTS components)
        _snodec_component_property(
            "${component}" RUNTIME_ONLY runtime_only
        )
        if(runtime_only)
            _snodec_materialize_runtime_component("${component}")
        else()
            _snodec_materialize_library_component("${component}")
        endif()
    endforeach()

    set_property(GLOBAL PROPERTY SNODEC_FINALIZED_COMPONENTS ${components})
    set_property(GLOBAL PROPERTY SNODEC_COMPONENTS_FINALIZED TRUE)
endfunction()

function(snodec_get_components result)
    _snodec_require_finalized("snodec_get_components")
    get_property(components GLOBAL PROPERTY SNODEC_FINALIZED_COMPONENTS)
    set(${result} "${components}" PARENT_SCOPE)
endfunction()

function(snodec_get_public_components result)
    _snodec_require_finalized("snodec_get_public_components")
    _snodec_public_components(public_components)
    set(${result} "${public_components}" PARENT_SCOPE)
endfunction()

function(snodec_get_component_dependency_definitions result)
    _snodec_require_finalized(
        "snodec_get_component_dependency_definitions"
    )
    _snodec_component_dependency_definitions(dependency_definitions)
    set(${result} "${dependency_definitions}" PARENT_SCOPE)
endfunction()

# Packaging adapters can enumerate snodec_get_components() and query this
# normalized metadata. Runtime/development install component names are stable
# inputs for both CPack and external systems such as OpenWrt package recipes.
function(snodec_get_component_metadata component result_prefix)
    _snodec_require_finalized("snodec_get_component_metadata")

    get_property(components GLOBAL PROPERTY SNODEC_FINALIZED_COMPONENTS)
    if(NOT component IN_LIST components)
        message(FATAL_ERROR "Unknown finalized SNode.C component '${component}'")
    endif()

    _snodec_component_property("${component}" DISPLAY_NAME display_name)
    _snodec_component_property("${component}" DESCRIPTION description)
    _snodec_component_property("${component}" TARGETS targets)
    _snodec_component_property("${component}" PUBLIC public_component)
    _snodec_component_property("${component}" RUNTIME_ONLY runtime_only)
    _snodec_component_property(
        "${component}" RESOLVED_RUNTIME_DEPENDS runtime_dependencies
    )
    _snodec_component_property(
        "${component}" RESOLVED_DEVELOPMENT_DEPENDS
        development_dependencies
    )
    _snodec_component_property(
        "${component}" DECLARATION_SOURCE_DIR declaration_source_dir
    )

    set("${result_prefix}_NAME" "${component}" PARENT_SCOPE)
    set("${result_prefix}_DISPLAY_NAME" "${display_name}" PARENT_SCOPE)
    set("${result_prefix}_DESCRIPTION" "${description}" PARENT_SCOPE)
    set("${result_prefix}_TARGETS" "${targets}" PARENT_SCOPE)
    set(
        "${result_prefix}_PUBLIC_COMPONENT" "${public_component}"
        PARENT_SCOPE
    )
    set("${result_prefix}_RUNTIME_ONLY" "${runtime_only}" PARENT_SCOPE)
    set(
        "${result_prefix}_RUNTIME_DEPENDS" "${runtime_dependencies}"
        PARENT_SCOPE
    )
    set(
        "${result_prefix}_DEVELOPMENT_DEPENDS"
        "${development_dependencies}"
        PARENT_SCOPE
    )
    set(
        "${result_prefix}_DECLARATION_SOURCE_DIR"
        "${declaration_source_dir}"
        PARENT_SCOPE
    )
    set(
        "${result_prefix}_RUNTIME_INSTALL_COMPONENT" "${component}"
        PARENT_SCOPE
    )
    if(runtime_only)
        set(
            "${result_prefix}_DEVELOPMENT_INSTALL_COMPONENT" ""
            PARENT_SCOPE
        )
    else()
        set(
            "${result_prefix}_DEVELOPMENT_INSTALL_COMPONENT"
            "${component}-dev"
            PARENT_SCOPE
        )
    endif()
endfunction()

macro(snodec_finalize_cpack_components)
    _snodec_require_finalized("snodec_finalize_cpack_components")

    get_property(
        _snodec_base_components GLOBAL PROPERTY SNODEC_FINALIZED_COMPONENTS
    )
    get_property(
        _snodec_referenced_install_components GLOBAL
        PROPERTY SNODEC_REFERENCED_INSTALL_COMPONENTS
    )
    list(REMOVE_DUPLICATES _snodec_referenced_install_components)

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
        _snodec_component_property(
            "${_snodec_component}"
            RESOLVED_RUNTIME_DEPENDS
            _snodec_runtime_dependencies
        )
        _snodec_component_property(
            "${_snodec_component}"
            RESOLVED_DEVELOPMENT_DEPENDS
            _snodec_development_dependencies
        )

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
                    "Install rules reference unregistered SNode.C component "
                    "'${_snodec_referenced_component}'"
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
                    "CMake install component '${_snodec_install_component}' "
                    "has no SNode.C CPack registration"
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
