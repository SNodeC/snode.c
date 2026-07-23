# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#
# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

cmake_minimum_required(VERSION 3.18)

set(AI_OPENAI_CODEX_BACKEND_CPP BackendCommand.cpp BackendCore.cpp
                                BackendState.cpp Reducer.cpp Snapshot.cpp
)

set(AI_OPENAI_CODEX_BACKEND_H
    BackendCommand.h
    BackendCore.h
    BackendEvent.h
    BackendState.h
    FrontendSession.h
    Reducer.h
    Snapshot.h
)

set(AI_OPENAI_CODEX_BACKEND_PRIVATE_H
    detail/PreserveUnmodeledTypedEvent.h
)

add_library(
    ai-openai-codex-backend SHARED ${AI_OPENAI_CODEX_BACKEND_CPP}
                                   ${AI_OPENAI_CODEX_BACKEND_H}
                                   ${AI_OPENAI_CODEX_BACKEND_PRIVATE_H}
)
add_library(snodec::ai-openai-codex-backend ALIAS ai-openai-codex-backend)

target_include_directories(
    ai-openai-codex-backend
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
           "$<INSTALL_INTERFACE:include>" ${CODEX_NLOHMANN_JSON_INCLUDE_DIRS}
)

target_compile_features(ai-openai-codex-backend PUBLIC cxx_std_20)

target_link_libraries(
    ai-openai-codex-backend
    PUBLIC ai-openai-codex
    PRIVATE core
)

set_target_properties(
    ai-openai-codex-backend
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-ai-openai-codex-backend
)

install(
    TARGETS ai-openai-codex-backend
    EXPORT snodec_ai-openai-codex-backend_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ai-openai-codex-backend
)

install(
    FILES ${AI_OPENAI_CODEX_BACKEND_H}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/ai/openai/codex/backend
    COMPONENT ai-openai-codex-backend
)

install(
    EXPORT snodec_ai-openai-codex-backend_Targets
    FILE snodec_ai-openai-codex-backend_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT ai-openai-codex-backend
)
