# snode.c - a slim toolkit for network communication
# Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

# Collect all currently added targets in all subdirectories
#
# Parameters:
# - RESULT the list containing all found targets
# - DIR root directory to start looking from
function(get_all_targets_dependencies RESULT DIR)
    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR IN LISTS SUBDIRS)
        get_all_targets_dependencies(${RESULT} "${SUBDIR}")
    endforeach()
    get_directory_property(TARGETS DIRECTORY "${DIR}" BUILDSYSTEM_TARGETS)
    if (TARGETS)
        unset(LIBRARY_DEPENDENCIES_LINE)
        foreach(TARGET IN LISTS TARGETS)
            unset(LIBRARY_DEPENDENCIES)
            get_target_property(TARGET_TYPE "${TARGET}" TYPE)
            if (${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
                get_target_property(TARGET_LIBRARY_DEPENDENCIES ${TARGET} INTERFACE_LINK_LIBRARIES)
            else()
                get_target_property(TARGET_LIBRARY_DEPENDENCIES ${TARGET} LINK_LIBRARIES)
            endif()
            if (NOT "${TARGET_LIBRARY_DEPENDENCIES}" STREQUAL "TARGET_LIBRARY_DEPENDENCIES-NOTFOUND")
                foreach(TARGET_LIBRARY_DEPENDENCY IN LISTS TARGET_LIBRARY_DEPENDENCIES)
                    if (TARGET "${TARGET_LIBRARY_DEPENDENCY}")
                        get_target_property(TARGET_LIBRARY_DEPENDENCY_type "${TARGET_LIBRARY_DEPENDENCY}" TYPE)
                        if(NOT "${TARGET_LIBRARY_DEPENDENCY_type}" STREQUAL "STATIC_LIBRARY")
                            list(APPEND LIBRARY_DEPENDENCIES ${TARGET_LIBRARY_DEPENDENCY})
                        endif()
                    endif()
                endforeach()
                if (LIBRARY_DEPENDENCIES)
                    if (NOT LIBRARY_DEPENDENCIES_LINE)
                        set(LIBRARY_DEPENDENCIES_LINE "set(${TARGET}_DEPENDENCIES ${LIBRARY_DEPENDENCIES})\n")
                    else()
                        string(APPEND LIBRARY_DEPENDENCIES_LINE " set(${TARGET}_DEPENDENCIES ${LIBRARY_DEPENDENCIES})\n")
                    endif()
                endif()
            endif()
        endforeach()
    endif()
    if (LIBRARY_DEPENDENCIES_LINE)
        set(${RESULT} "${${RESULT}} ${LIBRARY_DEPENDENCIES_LINE}" PARENT_SCOPE)
    else()
        set(${RESULT} "${${RESULT}}" PARENT_SCOPE)
    endif()
endfunction()
