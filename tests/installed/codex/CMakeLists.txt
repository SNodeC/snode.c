cmake_minimum_required(VERSION 3.18)

project(SNodeCInstalledCodexConsumer LANGUAGES CXX)

find_package(snodec REQUIRED COMPONENTS ai-openai-codex)

add_executable(SNodeCInstalledCodexConsumer CodexRawConsumer.cpp)
target_compile_features(SNodeCInstalledCodexConsumer PRIVATE cxx_std_20)
target_link_libraries(
    SNodeCInstalledCodexConsumer PRIVATE snodec::ai-openai-codex
)
