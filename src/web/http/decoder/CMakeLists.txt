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

cmake_minimum_required(VERSION 3.18)

set(DECODER_CPP Chunk.cpp)

set(DECODER_H Chunk.h)

add_library(http-decoder STATIC ${DECODER_CPP} ${DECODER_H})

set_property(TARGET http-decoder PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    http-decoder PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                        "$<INSTALL_INTERFACE:include/snode.c>"
)
