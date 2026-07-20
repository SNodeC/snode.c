# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#
# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT

cmake_minimum_required(VERSION 3.18)

set(AI_OPENAI_CODEX_FRONTEND_CPP
    BackendAdapter.cpp Codec.cpp EventCoalescer.cpp EventJournal.cpp
    Messages.cpp UpdateBatch.cpp
)

set(AI_OPENAI_CODEX_FRONTEND_H
    BackendAdapter.h
    Codec.h
    EventCoalescer.h
    EventJournal.h
    Messages.h
    Protocol.h
    UpdateBatch.h
)

add_library(
    ai-openai-codex-frontend SHARED ${AI_OPENAI_CODEX_FRONTEND_CPP}
                                    ${AI_OPENAI_CODEX_FRONTEND_H}
)
add_library(snodec::ai-openai-codex-frontend ALIAS ai-openai-codex-frontend)

target_include_directories(
    ai-openai-codex-frontend
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
           "$<INSTALL_INTERFACE:include>"
)

target_compile_features(ai-openai-codex-frontend PUBLIC cxx_std_20)

target_link_libraries(
    ai-openai-codex-frontend
    PUBLIC ai-openai-codex-backend
    PRIVATE core
)

set_target_properties(
    ai-openai-codex-frontend
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-ai-openai-codex-frontend
)

install(
    TARGETS ai-openai-codex-frontend
    EXPORT snodec_ai-openai-codex-frontend_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT ai-openai-codex-frontend
)

install(
    FILES ${AI_OPENAI_CODEX_FRONTEND_H}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/ai/openai/codex/frontend
    COMPONENT ai-openai-codex-frontend
)

install(
    EXPORT snodec_ai-openai-codex-frontend_Targets
    FILE snodec_ai-openai-codex-frontend_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT ai-openai-codex-frontend
)
