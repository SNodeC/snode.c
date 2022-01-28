cmake_minimum_required(VERSION 3.0)

add_executable(configtest ConfigTest.cpp)

target_include_directories(
    configtest
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_compile_definitions(
    configtest PRIVATE CONFFILEPATH="${CMAKE_CURRENT_SOURCE_DIR}"
    # PRIVATE CONFFILEPATH="/usr/local/etc/snodec"
)

install(TARGETS configtest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
