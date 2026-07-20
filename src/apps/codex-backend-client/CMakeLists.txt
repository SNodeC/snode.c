# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#
# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

cmake_minimum_required(VERSION 3.18)

add_library(
    codex-backend-client-support STATIC
    ClientConnection.cpp
    ClientConnection.h
    CommandDrainController.cpp
    CommandDrainController.h
    CodexBackendClientSocketContext.cpp
    CodexBackendClientSocketContext.h
    CodexBackendClientSocketContextFactory.cpp
    CodexBackendClientSocketContextFactory.h
    CommandParser.cpp
    CommandParser.h
    Configuration.cpp
    Configuration.h
    JsonLineFramer.cpp
    JsonLineFramer.h
    Presenter.cpp
    Presenter.h
    StdinReader.cpp
    StdinReader.h
)
target_include_directories(
    codex-backend-client-support
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)
target_compile_features(codex-backend-client-support PUBLIC cxx_std_20)
target_link_libraries(
    codex-backend-client-support PUBLIC ai-openai-codex-frontend
                                        net-un-stream-legacy core
)

add_executable(codex-backend-client main.cpp)
target_compile_features(codex-backend-client PRIVATE cxx_std_20)
target_link_libraries(codex-backend-client PRIVATE codex-backend-client-support)

install(TARGETS codex-backend-client RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                             COMPONENT apps
)
