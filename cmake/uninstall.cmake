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

add_custom_target(
    uninstall
    COMMAND sudo xargs rm -v < install_manifest.txt
    COMMAND sudo xargs -L1 dirname < install_manifest.txt | sort | uniq | sudo
            xargs rmdir -v --ignore-fail-on-non-empty -p
    COMMENT "Uninstall project"
)

# xargs rm < install_manifest.txt 140
