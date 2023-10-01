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

# these are cache variables, so they could be overwritten with -D,
set(CPACK_PACKAGE_NAME
    ${PROJECT_NAME}
    CACHE STRING "The resulting package name"
)

# set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple Node in C++" CACHE STRING
# "Package description for the package metadata" )

set(CPACK_PACKAGE_CONTACT "me@vchrist.at")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER
    "Volker Christian <${CPACK_PACKAGE_CONTACT}>"
)
set(CPACK_PACKAGE_VENDOR "Volker Christian")

set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_STRIP_FILES YES)

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/_packages")

# https://unix.stackexchange.com/a/11552/254512
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
# /${CMAKE_PROJECT_VERSION}")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
set(CPACK_DEB_COMPONENT_INSTALL YES)

get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL notneeded)

include(CPack)

cpack_add_component(logger)
cpack_add_component(utils DEPENDS logger)

cpack_add_component(mux-epoll)
cpack_add_component(mux-poll)
cpack_add_component(mux-select)

cpack_add_component(core DEPENDS mux-${IO_Multiplexer} utils)
cpack_add_component(core-socket DEPENDS core)
cpack_add_component(core-socket-stream DEPENDS core-socket)
cpack_add_component(core-socket-stream-legacy DEPENDS core-socket-stream)
cpack_add_component(core-socket-stream-tls DEPENDS core-socket-stream)

cpack_add_component(net)

cpack_add_component(net-in DEPENDS net)
cpack_add_component(net-in6 DEPENDS net)
cpack_add_component(net-l2 DEPENDS net)
cpack_add_component(net-rc DEPENDS net)
cpack_add_component(net-un DEPENDS net)

cpack_add_component(net-in-stream DEPENDS net-in)
cpack_add_component(net-in6-stream DEPENDS net-in6)
cpack_add_component(net-l2-stream DEPENDS net-l2)
cpack_add_component(net-rc-stream DEPENDS net-rc)
cpack_add_component(net-un-stream DEPENDS net-un)

cpack_add_component(
    net-in-stream-legacy DEPENDS net-in-stream core-socket-stream-legacy
)
cpack_add_component(
    net-in6-stream-legacy DEPENDS net-in6-stream core-socket-stream-legacy
)
cpack_add_component(
    net-l2-stream-legacy DEPENDS net-l2-stream core-socket-stream-legacy
)
cpack_add_component(
    net-rc-stream-legacy DEPENDS net-rc-stream core-socket-stream-legacy
)
cpack_add_component(
    net-un-stream-legacy DEPENDS net-un-stream core-socket-stream-legacy
)

cpack_add_component(
    net-in-stream-tls DEPENDS net-in-stream core-socket-stream-tls
)
cpack_add_component(
    net-in6-stream-tls DEPENDS net-in6-stream core-socket-stream-tls
)
cpack_add_component(
    net-l2-stream-tls DEPENDS net-l2-stream core-socket-stream-tls
)
cpack_add_component(
    net-rc-stream-tls DEPENDS net-rc-stream core-socket-stream-tls
)
cpack_add_component(
    net-un-stream-tls DEPENDS net-un-stream core-socket-stream-tls
)

cpack_add_component(net-un-dgram DEPENDS net-un)

cpack_add_component(http)
cpack_add_component(http-server DEPENDS http)
cpack_add_component(http-client DEPENDS http)
cpack_add_component(http-server-express DEPENDS http-server)

cpack_add_component(websocket)
cpack_add_component(websocket-server DEPENDS websocket http-server)
cpack_add_component(websocket-client DEPENDS websocket http-client)

cpack_add_component(mqtt)
cpack_add_component(mqtt-server DEPENDS mqtt)
cpack_add_component(mqtt-client DEPENDS mqtt)

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
