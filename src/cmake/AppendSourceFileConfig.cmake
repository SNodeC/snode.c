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

function(append_source_file_config SOURCE_FILE CONFIG_OPTION DESCRIPTION DEFAULT_VALUE)
    if ("${${CONFIG_OPTION}}" STREQUAL "")
        set(VALUE ${DEFAULT_VALUE})
    elseif("${ARGN}" STREQUAL "")
        set(VALUE ${${CONFIG_OPTION}})
    else()
        set(VALUE ${ARGN})
    endif()
    set(SNODEC_${CONFIG_OPTION}
        ${VALUE}
        CACHE STRING "${DESCRIPTION}"
    )
    message(STATUS "Adding compile definition '" ${CONFIG_OPTION}=${SNODEC_${CONFIG_OPTION}} "' for '${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}'")
    set_property(
      SOURCE ${SOURCE_FILE}
      APPEND PROPERTY COMPILE_DEFINITIONS ${CONFIG_OPTION}=${SNODEC_${CONFIG_OPTION}}
    )
endfunction(append_source_file_config SOURCE_FILE CONFIG_OPTION DESCRIPTION DEFAULT_VALUE)
