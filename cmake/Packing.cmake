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

set(CPACK_PACKAGE_NAME
    ${PROJECT_NAME}
    CACHE STRING "The resulting package name"
)
set(CPACK_PACKAGE_CONTACT "me@vchrist.at")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER
    "Volker Christian <${CPACK_PACKAGE_CONTACT}>"
)
set(CPACK_PACKAGE_VENDOR "Volker Christian")

option(SNODEC_PACKAGE_STRIP "Strip binaries in generated packages" OFF)
set(SNODEC_PACKAGE_INSTALL_PREFIX
    "/usr"
    CACHE PATH "Installation prefix inside generated binary packages"
)

set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_STRIP_FILES ${SNODEC_PACKAGE_STRIP})
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/_packages")
set(CPACK_PACKAGING_INSTALL_PREFIX "${SNODEC_PACKAGE_INSTALL_PREFIX}")

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
set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_DEB_COMPONENT_INSTALL YES)

# Component ownership and dependency metadata are registered next to the
# corresponding targets. This final step only validates and materializes that
# distributed component model for CPack.
snodec_finalize_cpack_components()

include(CPack)
