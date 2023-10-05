# snode.c - a slim toolkit for network communication Copyright (C) 2020, 2021,
# 2022, 2023 Volker Christian <me@vchrist.at>
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

cmake_minimum_required(VERSION 3.18)

set(TOWERCALCULATION_CPP towerCalculation.cpp KeyboardReader.cpp
                         TowerCalculator.cpp
)

set(TOWERCALCULATION_H KeyboardReader.h TowerCalculator.h)

add_executable(towerCalculation ${TOWERCALCULATION_CPP} ${TOWERCALCULATION_H})

target_link_libraries(towerCalculation PRIVATE core)

install(TARGETS towerCalculation RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                         COMPONENT apps
)
