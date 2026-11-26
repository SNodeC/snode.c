cmake_minimum_required(VERSION 3.18)

project(SNodeCPackageInstalledConsumer LANGUAGES CXX)

include(CheckPIESupported)
check_pie_supported()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(
    snodec CONFIG REQUIRED
    COMPONENTS jsonrpc
               jsonrpc-server-websocket
               jsonrpc-client-websocket
               mcp
               mcp-client
               mcp-client-http
               mcp-server
               mcp-server-http
               core
               net-stdio-stream
               net-in-stream-legacy
               net-un-stream-legacy
               http-server-express-legacy-in
)

if(TARGET snodec::mcp-server-stream)
    message(FATAL_ERROR "MCP stream must belong to the mcp-server component")
endif()

add_executable(consumer main.cpp)
target_compile_features(consumer PRIVATE cxx_std_20)
target_link_libraries(
    consumer
    PRIVATE snodec::jsonrpc
            snodec::jsonrpc-server-websocket
            snodec::jsonrpc-client-websocket
            snodec::mcp
            snodec::mcp-client
            snodec::mcp-client-http
            snodec::mcp-server
            snodec::mcp-server-http
            snodec::core
            snodec::net-stdio-stream
            snodec::net-in-stream-legacy
            snodec::net-un-stream-legacy
            snodec::http-server-express-legacy-in
)

# Compile the exact application feature classes against installed public
# headers.
set(mcp_app_sources "${CMAKE_CURRENT_SOURCE_DIR}/../../src/apps/mcp")
add_library(
    mcp-features STATIC
    ${mcp_app_sources}/FeatureContent.cpp
    ${mcp_app_sources}/FeatureInteractions.cpp
    ${mcp_app_sources}/FeatureResources.cpp
)
target_include_directories(mcp-features PUBLIC ${mcp_app_sources})
target_link_libraries(mcp-features PUBLIC snodec::mcp-server)
add_executable(mcp-feature-consumer features.cpp)
target_link_libraries(mcp-feature-consumer PRIVATE mcp-features)
add_executable(
    mcp-tour-consumer
    ${mcp_app_sources}/client.cpp ${mcp_app_sources}/Client.cpp
    ${mcp_app_sources}/ClientConfig.cpp
)
target_link_libraries(mcp-tour-consumer PRIVATE snodec::mcp-client-http)
