# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026

cmake_minimum_required(VERSION 3.18)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CODEX_NLOHMANN_JSON REQUIRED nlohmann_json>=3.11)

set(AI_OPENAI_CODEX_CPP
    AppServerClient.cpp
    detail/CodexErrorInfoCodec.cpp
    detail/ClientOperationCodec.cpp
    detail/ConversationCodec.cpp
    detail/EventDecoder.cpp
    detail/ItemDecoder.cpp
    detail/ProtocolCodec.cpp
    detail/ProtocolSurfaceRegistry.cpp
    detail/ServerRequestDecoder.cpp
    detail/ThreadCodec.cpp
    detail/Transport.cpp
    detail/TurnCodec.cpp
    stdio/Client.cpp
    stdio/StdioTransport.cpp
    typed/Client.cpp
    typed/Types.cpp
)

set(AI_OPENAI_CODEX_H
    AppServerClient.h
    Protocol.h
    detail/CodexErrorInfoCodec.h
    detail/ClientOperationCodec.h
    detail/ConversationCodec.h
    detail/ConversationUnionCodecDescriptors.inc
    detail/ThreadItemCodecDescriptors.inc
    detail/ResponseItemCodecDescriptors.inc
    detail/ServerNotificationCodecDescriptors.inc
    detail/DecodeDiagnostic.h
    detail/EventDecoder.h
    detail/ItemDecoder.h
    detail/ProtocolCodec.h
    detail/ProtocolSurfaceRegistry.h
    detail/ProtocolSurfaceRegistryData.inc
    detail/ServerRequestDecoder.h
    detail/ThreadCodec.h
    detail/Transport.h
    detail/TurnCodec.h
    stdio/Client.h
    stdio/StdioTransport.h
    typed/Client.h
    typed/CodexErrorInfo.h
    typed/Conversation.h
    typed/Events.h
    typed/Items.h
    typed/Results.h
    typed/ServerRequests.h
    typed/Threads.h
    typed/Turns.h
    typed/Types.h
)

add_library(ai-openai-codex SHARED ${AI_OPENAI_CODEX_CPP} ${AI_OPENAI_CODEX_H})
add_library(snodec::ai-openai-codex ALIAS ai-openai-codex)

target_include_directories(
    ai-openai-codex
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
           ${CODEX_NLOHMANN_JSON_INCLUDE_DIRS}
)

target_compile_features(ai-openai-codex PUBLIC cxx_std_20)

target_link_libraries(ai-openai-codex PRIVATE core)

set_target_properties(
    ai-openai-codex
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-ai-openai-codex
)

install(
    TARGETS ai-openai-codex
    EXPORT snodec_ai-openai-codex_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ai-openai-codex
)

install(
    FILES AppServerClient.h Protocol.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/ai/openai/codex
    COMPONENT ai-openai-codex
)

install(
    FILES typed/Client.h
          typed/CodexErrorInfo.h
          typed/Conversation.h
          typed/Events.h
          typed/Items.h
          typed/Results.h
          typed/ServerRequests.h
          typed/Threads.h
          typed/Turns.h
          typed/Types.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/ai/openai/codex/typed
    COMPONENT ai-openai-codex
)

install(
    FILES stdio/Client.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/ai/openai/codex/stdio
    COMPONENT ai-openai-codex
)

install(
    EXPORT snodec_ai-openai-codex_Targets
    FILE snodec_ai-openai-codex_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT ai-openai-codex
)

add_subdirectory(backend)
add_subdirectory(frontend)
