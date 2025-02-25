# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025
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

# Collect all currently added targets in all subdirectories
function(get_all_targets_dependencies RESULT DIR)
    get_property(SUBDIRECTORIES DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIRECTORIE IN LISTS SUBDIRECTORIES)
        get_all_targets_dependencies(${RESULT} "${SUBDIRECTORIE}")
    endforeach(SUBDIRECTORIE IN LISTS SUBDIRECTORIES)

    get_directory_property(TARGETS DIRECTORY "${DIR}" BUILDSYSTEM_TARGETS)
    foreach(TARGET IN LISTS TARGETS)
        unset(LIBRARY_DEPENDENCIES)
        get_target_property(TARGET_TYPE "${TARGET}" TYPE)
        if (${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
            get_target_property(TARGET_LIBRARY_DEPENDENCIES ${TARGET} INTERFACE_LINK_LIBRARIES)
        else(${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
            get_target_property(TARGET_LIBRARY_DEPENDENCIES ${TARGET} LINK_LIBRARIES)
        endif(${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
        if (NOT "${TARGET_LIBRARY_DEPENDENCIES}" STREQUAL "TARGET_LIBRARY_DEPENDENCIES-NOTFOUND")
            foreach(TARGET_LIBRARY_DEPENDENCY IN LISTS TARGET_LIBRARY_DEPENDENCIES)
                if (TARGET "${TARGET_LIBRARY_DEPENDENCY}")
                    get_target_property(TARGET_LIBRARY_DEPENDENCY_type "${TARGET_LIBRARY_DEPENDENCY}" TYPE)
                    if(NOT "${TARGET_LIBRARY_DEPENDENCY_type}" STREQUAL "STATIC_LIBRARY")
                        list(APPEND LIBRARY_DEPENDENCIES ${TARGET_LIBRARY_DEPENDENCY})
                    endif(NOT "${TARGET_LIBRARY_DEPENDENCY_type}" STREQUAL "STATIC_LIBRARY")
                endif(TARGET "${TARGET_LIBRARY_DEPENDENCY}")
            endforeach()
            if (LIBRARY_DEPENDENCIES)
                if (NOT LIBRARY_DEPENDENCIES_LINES)
                    set(LIBRARY_DEPENDENCIES_LINES "set(${TARGET}_DEPENDENCIES ${LIBRARY_DEPENDENCIES})")
                else(NOT LIBRARY_DEPENDENCIES_LINES)
                    string(APPEND LIBRARY_DEPENDENCIES_LINES "\nset(${TARGET}_DEPENDENCIES ${LIBRARY_DEPENDENCIES})")
                endif(NOT LIBRARY_DEPENDENCIES_LINES)
            endif(LIBRARY_DEPENDENCIES)
        endif(NOT "${TARGET_LIBRARY_DEPENDENCIES}" STREQUAL "TARGET_LIBRARY_DEPENDENCIES-NOTFOUND")
    endforeach(TARGET IN LISTS TARGETS)

    if (LIBRARY_DEPENDENCIES_LINES)
        if (${RESULT})
            string(APPEND ${RESULT} "\n")
        endif(${RESULT})
        string(APPEND ${RESULT} "${LIBRARY_DEPENDENCIES_LINES}")
    endif()

    set(${RESULT} "${${RESULT}}" PARENT_SCOPE)
endfunction()
