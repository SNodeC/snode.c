# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
find_package(nlohmann_json 3.11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MCP_JSON REQUIRED nlohmann_json>=3.11)
pkg_check_modules(MCP_PCRE2 REQUIRED IMPORTED_TARGET libpcre2-8)

set(MCP_SAVED_IWYU "${CMAKE_CXX_INCLUDE_WHAT_YOU_USE}")
unset(CMAKE_CXX_INCLUDE_WHAT_YOU_USE)
set(JSON_VALIDATOR_INSTALL
    OFF
    CACHE BOOL "" FORCE
)
set(JSON_VALIDATOR_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE
)
set(JSON_VALIDATOR_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE
)
set(JSON_VALIDATOR_SHARED_LIBS
    OFF
    CACHE BOOL "" FORCE
)
set(JSON_VALIDATOR_USE_PCRE2
    ON
    CACHE BOOL "" FORCE
)
add_subdirectory(json-schema-validator)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${MCP_SAVED_IWYU}")
set_target_properties(
    nlohmann_json_schema_validator PROPERTIES POSITION_INDEPENDENT_CODE ON
)
target_compile_options(
    nlohmann_json_schema_validator
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-error>
            $<$<CXX_COMPILER_ID:Clang>:-Wno-error>
)

add_library(mcp SHARED Results.cpp Schema.cpp Types.cpp Utilities.cpp)
add_library(snodec::mcp ALIAS mcp)
target_compile_features(mcp PUBLIC cxx_std_20)
target_link_libraries(
    mcp
    PUBLIC nlohmann_json::nlohmann_json
    PRIVATE nlohmann_json_schema_validator PkgConfig::MCP_PCRE2
)
target_include_directories(
    mcp SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/json-schema-validator/src"
)
target_include_directories(
    mcp PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
)
set_target_properties(
    mcp
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_MCP_SOVERSION}
               OUTPUT_NAME snodec-mcp
               INSTALL_RPATH "$ORIGIN"
)
install(
    TARGETS mcp
    EXPORT snodec_mcp_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mcp COMPONENT mcp
)
install(
    FILES Types.h Results.h Protocol.h Schema.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp
    COMPONENT mcp
)
install(
    EXPORT snodec_mcp_Targets
    FILE snodec_mcp_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp
)
install(
    FILES cmake/snodec_mcp_Targets_Config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp
)
add_subdirectory(server)
