# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#
# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

cmake_minimum_required(VERSION 3.18)

add_library(
    codex-backend-unix-adapter STATIC
    CodexFrontendSocketContext.cpp
    CodexFrontendSocketContext.h
    CodexFrontendSocketContextFactory.cpp
    CodexFrontendSocketContextFactory.h
    Configuration.cpp
    Configuration.h
    JsonLineFramer.cpp
    JsonLineFramer.h
)
target_include_directories(
    codex-backend-unix-adapter
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)
target_compile_features(codex-backend-unix-adapter PUBLIC cxx_std_20)
target_link_libraries(
    codex-backend-unix-adapter
    PUBLIC ai-openai-codex-frontend net-un-stream-legacy
    PRIVATE core
)

add_executable(codex-backend main.cpp)
target_compile_features(codex-backend PRIVATE cxx_std_20)
target_link_libraries(
    codex-backend
    PRIVATE codex-backend-unix-adapter ai-openai-codex ai-openai-codex-backend
            ai-openai-codex-frontend net-un-stream-legacy core
)

install(TARGETS codex-backend RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                      COMPONENT apps
)
