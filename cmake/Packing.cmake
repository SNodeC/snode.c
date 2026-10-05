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

# these are cache variables, so they could be overwritten with -D,
set(CPACK_PACKAGE_NAME
    snodec
    CACHE STRING "The resulting package name"
)

set(CPACK_PACKAGE_CONTACT "me@vchrist.at")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER
    "Volker Christian <${CPACK_PACKAGE_CONTACT}>"
)
set(CPACK_PACKAGE_VENDOR "Volker Christian")

set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_STRIP_FILES YES)

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/_packages")

set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Source packages are reproducibility artifacts. Keep local build products and
# execution-environment metadata out of those archives. CPack's default ignore
# list does not exclude an in-tree build.
set(
    CPACK_SOURCE_IGNORE_FILES
    "/CVS/"
    "/\\.svn/"
    "/\\.bzr/"
    "/\\.hg/"
    "/\\.git/"
    "/\\.agents/"
    "/\\.codex/"
    "/\\.cache/"
    "/\\.kdev4/"
    "/\\.qtcreator/"
    "/\\.vscode/"
    "/_CPack_Packages/"
    "/build[^/]*/"
    "/softwipe_build/"
    "/test1-cppcheck-build-dir/"
    "/__pycache__/"
    "\\.kdev4$"
    "\\.py[cod]$"
    "\\.swp$"
    "\\.#"
    "/#"
    "~$"
)

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
set(CPACK_DEB_COMPONENT_INSTALL YES)

# The full-install package is a regular component, owned by this project.
include(GNUInstallDirs)
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/${CPACK_PACKAGE_NAME}" COMPONENT full)
set(CPACK_DEBIAN_FULL_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_RPM_FULL_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_RELEASE_DIST OFF)
set(CPACK_RPM_PACKAGE_RELOCATABLE OFF)
set(CPACK_RPM_INSTALL_WITH_EXEC ON)
file(STRINGS "${CMAKE_SOURCE_DIR}/LICENSE" license REGEX "^SPDX-License-Identifier: " LIMIT_COUNT 1)
string(REPLACE "SPDX-License-Identifier: " "" CPACK_RPM_PACKAGE_LICENSE "${license}")
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/PackageConfig.cmake")

install(DIRECTORY DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/snode.c" COMPONENT common)
get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL notneeded)
list(SORT CPACK_COMPONENTS_ALL)

# Resolve built libraries without requiring an existing SNode.C installation.
foreach(component IN LISTS CPACK_COMPONENTS_ALL)
    if(TARGET ${component})
        get_target_property(component_libdir ${component} LIBRARY_OUTPUT_DIRECTORY)
        if(NOT component_libdir)
            get_target_property(component_libdir ${component} BINARY_DIR)
        endif()
        list(APPEND CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "${component_libdir}")
    endif()
endforeach()
list(REMOVE_DUPLICATES CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS)

set(full_dependencies ${CPACK_COMPONENTS_ALL})
list(REMOVE_ITEM full_dependencies full)
set(CPACK_DEBIAN_COMMON_PACKAGE_DEPENDS adduser)
set(CPACK_DEBIAN_COMMON_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_LIST_DIR}/debian/postinst")
set(CPACK_RPM_LOGGER_PACKAGE_REQUIRES "snodec-common")
set(CPACK_RPM_CONTROL_PACKAGE_REQUIRES "snodec-common")
set(CPACK_RPM_COMMON_PACKAGE_REQUIRES_POST "shadow-utils, glibc")
set(CPACK_RPM_COMMON_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_LIST_DIR}/rpm/postinst")
include(CPack)
cpack_add_component(full DEPENDS ${full_dependencies})

cpack_add_component(common)
cpack_add_component(control DEPENDS common)
cpack_add_component(logger DEPENDS common)
cpack_add_component(utils DEPENDS logger)

cpack_add_component(mux-epoll)
cpack_add_component(mux-poll)
cpack_add_component(mux-select)

cpack_add_component(core DEPENDS mux-${SNODEC_IO_MULTIPLEXER} utils)
cpack_add_component(core-socket DEPENDS core)
cpack_add_component(core-socket-stream DEPENDS core-socket)
cpack_add_component(core-socket-stream-legacy DEPENDS core-socket-stream)
cpack_add_component(core-socket-stream-tls DEPENDS core-socket-stream)

cpack_add_component(net DEPENDS core-socket)

foreach(family IN ITEMS in in6 l2 rc un)
    cpack_add_component(net-${family} DEPENDS net)
    cpack_add_component(net-${family}-phy DEPENDS net-${family})
    cpack_add_component(net-${family}-phy-stream DEPENDS net-${family}-phy)
    cpack_add_component(net-${family}-stream DEPENDS net-${family}-phy-stream)
    cpack_add_component(
        net-${family}-stream-legacy DEPENDS net-${family}-stream
                                            core-socket-stream-legacy
    )
    cpack_add_component(
        net-${family}-stream-tls DEPENDS net-${family}-stream
                                         core-socket-stream-tls
    )
endforeach()

cpack_add_component(net-un-dgram DEPENDS net-un-phy)

cpack_add_component(http DEPENDS core-socket-stream)
cpack_add_component(http-server DEPENDS http)
cpack_add_component(http-client DEPENDS http)
cpack_add_component(http-server-express DEPENDS http-server)

foreach(family IN ITEMS in in6 rc un)
    foreach(transport IN ITEMS legacy tls)
        cpack_add_component(
            http-server-express-${transport}-${family}
            DEPENDS http-server-express net-${family}-stream-${transport}
        )
    endforeach()
endforeach()

cpack_add_component(websocket DEPENDS utils)
cpack_add_component(websocket-server DEPENDS websocket http-server)
cpack_add_component(websocket-client DEPENDS websocket http-client)

cpack_add_component(mqtt DEPENDS core-socket-stream)
cpack_add_component(mqtt-server DEPENDS mqtt)
cpack_add_component(mqtt-client DEPENDS mqtt)

cpack_add_component(mqtt-server-websocket DEPENDS mqtt-server websocket-server)
cpack_add_component(mqtt-client-websocket DEPENDS mqtt-client websocket-client)

cpack_add_component(mqtt-fast)

cpack_add_component(db-mariadb DEPENDS core)

cpack_add_component(
    apps
    DISPLAY_NAME "Applications"
    DESCRIPTION "We install Applications"
    DEPENDS http-server-express
            http-client
            net-in-stream-tls
            net-in6-stream-tls
            net-l2-stream-tls
            net-rc-stream-tls
            net-un-stream-tls
            net-in-stream-legacy
            net-in6-stream-legacy
            net-l2-stream-legacy
            net-rc-stream-legacy
            net-un-stream-legacy
            core-socket-stream-legacy
            core-socket-stream-tls
            websocket-server
            websocket-client
            db-mariadb
)
