cmake_minimum_required(VERSION 3.18)

project(SNodeCInstalledCodexConsumer LANGUAGES CXX)

find_package(
    snodec REQUIRED COMPONENTS ai-openai-codex ai-openai-codex-backend
                               ai-openai-codex-frontend
)

add_executable(SNodeCInstalledCodexConsumer CodexRawConsumer.cpp)
target_compile_features(SNodeCInstalledCodexConsumer PRIVATE cxx_std_20)
target_link_libraries(
    SNodeCInstalledCodexConsumer PRIVATE snodec::ai-openai-codex
)

add_executable(SNodeCInstalledCodexTypedConsumer CodexTypedConsumer.cpp)
target_compile_features(SNodeCInstalledCodexTypedConsumer PRIVATE cxx_std_20)
target_link_libraries(
    SNodeCInstalledCodexTypedConsumer PRIVATE snodec::ai-openai-codex
)

add_executable(
    SNodeCInstalledCodexBackendFrontendConsumer
    CodexBackendFrontendConsumer.cpp
)
target_compile_features(
    SNodeCInstalledCodexBackendFrontendConsumer PRIVATE cxx_std_20
)
target_link_libraries(
    SNodeCInstalledCodexBackendFrontendConsumer
    PRIVATE snodec::ai-openai-codex-backend snodec::ai-openai-codex-frontend
)
