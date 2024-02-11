# SNode.C - a slim toolkit for network communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

find_program(iwyu_path NAMES include-what-you-use iwyu)
if(iwyu_path)
    option(CHECK_INCLUDES "Check used headers")

    if(CHECK_INCLUDES)
        set(iwyu_path_and_options
            ${iwyu_path}
            -Xiwyu
            --verbose=3
            -Xiwyu
            --cxx17ns
            -Xiwyu
            --regex=ecmascript
            -Xiwyu
            --quoted_includes_first
            -Xiwyu
            --max_line_length=160
        )

        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
    endif(CHECK_INCLUDES)
else(iwyu_path)
    message(
        WARNING
            "Include-what-you-use (iwyu) is needed for checking include dependencies"
    )
endif(iwyu_path)
