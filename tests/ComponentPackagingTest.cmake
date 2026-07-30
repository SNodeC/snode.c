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

if(NOT SNODEC_BUILD_DIR)
    message(FATAL_ERROR "SNODEC_BUILD_DIR is required")
endif()
if(NOT SNODEC_INSTALL_LIBDIR)
    message(FATAL_ERROR "SNODEC_INSTALL_LIBDIR is required")
endif()
if(NOT SNODEC_INSTALL_INCLUDEDIR)
    message(FATAL_ERROR "SNODEC_INSTALL_INCLUDEDIR is required")
endif()
if(NOT DEFINED SNODEC_SHARED_LIBRARY_PREFIX)
    message(FATAL_ERROR "SNODEC_SHARED_LIBRARY_PREFIX is required")
endif()
if(NOT DEFINED SNODEC_SHARED_LIBRARY_SUFFIX)
    message(FATAL_ERROR "SNODEC_SHARED_LIBRARY_SUFFIX is required")
endif()

set(stage "${SNODEC_BUILD_DIR}/component-packaging-test")
set(runtime_prefix "${stage}/runtime")
set(development_prefix "${stage}/development")
file(REMOVE_RECURSE "${stage}")

function(install_component component prefix)
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" --install "${SNODEC_BUILD_DIR}" --prefix
            "${prefix}" --component "${component}"
        RESULT_VARIABLE install_result
        OUTPUT_VARIABLE install_output
        ERROR_VARIABLE install_error
    )
    if(NOT install_result EQUAL 0)
        message(
            FATAL_ERROR
                "installation of component '${component}' failed\n"
                "${install_output}\n${install_error}"
        )
    endif()
endfunction()

install_component(core "${runtime_prefix}")

set(
    core_library_name
    "${SNODEC_SHARED_LIBRARY_PREFIX}snodec-core${SNODEC_SHARED_LIBRARY_SUFFIX}"
)
set(
    core_library
    "${runtime_prefix}/${SNODEC_INSTALL_LIBDIR}/${core_library_name}"
)
if(EXISTS "${core_library}" OR IS_SYMLINK "${core_library}")
    message(FATAL_ERROR "runtime component contains development namelink")
endif()
file(
    GLOB versioned_core_libraries
    LIST_DIRECTORIES FALSE
    "${core_library}.*"
)
if(NOT versioned_core_libraries)
    message(FATAL_ERROR "runtime component contains no versioned core library")
endif()
file(
    GLOB_RECURSE runtime_headers
    LIST_DIRECTORIES FALSE
    "${runtime_prefix}/${SNODEC_INSTALL_INCLUDEDIR}/*"
)
if(runtime_headers)
    message(FATAL_ERROR "runtime component contains headers: ${runtime_headers}")
endif()
file(
    GLOB_RECURSE runtime_cmake_files
    LIST_DIRECTORIES FALSE
    "${runtime_prefix}/${SNODEC_INSTALL_LIBDIR}/cmake/*"
)
if(runtime_cmake_files)
    message(
        FATAL_ERROR
            "runtime component contains CMake package files: ${runtime_cmake_files}"
    )
endif()

install_component(core-dev "${development_prefix}")

set(
    development_core_library
    "${development_prefix}/${SNODEC_INSTALL_LIBDIR}/${core_library_name}"
)
if(NOT EXISTS "${development_core_library}"
   AND NOT IS_SYMLINK "${development_core_library}"
)
    message(FATAL_ERROR "development component contains no core namelink")
endif()
file(
    GLOB development_versioned_core_libraries
    LIST_DIRECTORIES FALSE
    "${development_core_library}.*"
)
if(development_versioned_core_libraries)
    message(
        FATAL_ERROR
            "development component contains versioned runtime libraries: "
            "${development_versioned_core_libraries}"
    )
endif()
if(NOT EXISTS
   "${development_prefix}/${SNODEC_INSTALL_INCLUDEDIR}/snode.c/core/SNodeC.h"
)
    message(FATAL_ERROR "development component contains no public core headers")
endif()
if(NOT EXISTS
   "${development_prefix}/${SNODEC_INSTALL_LIBDIR}/cmake/snodec/snodecConfig.cmake"
)
    message(FATAL_ERROR "development component contains no package config")
endif()
if(NOT EXISTS
   "${development_prefix}/${SNODEC_INSTALL_LIBDIR}/cmake/snodec/snodec_core_Targets.cmake"
)
    message(FATAL_ERROR "development component contains no core target export")
endif()

include("${SNODEC_BUILD_DIR}/CPackConfig.cmake")
foreach(
    expected_component
    IN ITEMS core core-dev net-un-phy-stream net-un-phy-stream-dev
)
    list(FIND CPACK_COMPONENTS_ALL "${expected_component}" component_index)
    if(component_index EQUAL -1)
        message(
            FATAL_ERROR
                "generated CPack model misses component '${expected_component}'"
        )
    endif()
endforeach()
foreach(removed_component IN ITEMS net-un-sphy-tream mqtt-fast)
    list(FIND CPACK_COMPONENTS_ALL "${removed_component}" component_index)
    if(NOT component_index EQUAL -1)
        message(
            FATAL_ERROR
                "generated CPack model contains invalid component '${removed_component}'"
        )
    endif()
endforeach()

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

set(core_development_dependencies_var "CPACK_COMPONENT_CORE-DEV_DEPENDS")
foreach(expected_dependency IN ITEMS core utils-dev)
    list(
        FIND ${core_development_dependencies_var} "${expected_dependency}"
        dependency_index
    )
    if(dependency_index EQUAL -1)
        message(
            FATAL_ERROR
                "core-dev misses dependency '${expected_dependency}': "
                "${${core_development_dependencies_var}}"
        )
    endif()
endforeach()
set(selected_multiplexer_development "${selected_multiplexer_component}-dev")
list(
    FIND ${core_development_dependencies_var}
    "${selected_multiplexer_development}" dependency_index
)
if(NOT dependency_index EQUAL -1)
    message(
        FATAL_ERROR
            "core-dev exposes private multiplexer dependency "
            "'${selected_multiplexer_development}'"
    )
endif()

if(NOT CPACK_PACKAGING_INSTALL_PREFIX STREQUAL "/usr")
    message(
        FATAL_ERROR
            "binary packages use unexpected installation prefix "
            "'${CPACK_PACKAGING_INSTALL_PREFIX}'"
    )
endif()
