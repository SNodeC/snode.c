if(NOT SNODEC_BUILD_DIR)
    message(FATAL_ERROR "SNODEC_BUILD_DIR is required")
endif()
if(NOT SNODEC_INSTALL_LIBDIR)
    message(FATAL_ERROR "SNODEC_INSTALL_LIBDIR is required")
endif()
if(NOT SNODEC_INSTALL_INCLUDEDIR)
    message(FATAL_ERROR "SNODEC_INSTALL_INCLUDEDIR is required")
endif()

set(stage "${SNODEC_BUILD_DIR}/staged-installed-consumer")
set(prefix "${stage}/prefix")
set(consumer "${stage}/consumer.cpp")
set(include_directory "${prefix}/${SNODEC_INSTALL_INCLUDEDIR}/snode.c")
set(library_directory "${prefix}/${SNODEC_INSTALL_LIBDIR}")
set(http_library_directory "${library_directory}/snode.c/web/http")
file(REMOVE_RECURSE "${stage}")
file(MAKE_DIRECTORY "${stage}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${SNODEC_BUILD_DIR}" --prefix
            "${prefix}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error
)
if(NOT install_result EQUAL 0)
    message(
        FATAL_ERROR "staged install failed\n${install_output}\n${install_error}"
    )
endif()

foreach(private_header IN
        ITEMS core/EventLoop.h core/EventMultiplexer.h
              core/DescriptorEventPublisher.h core/TimerEventPublisher.h
)
    if(EXISTS "${include_directory}/${private_header}")
        message(FATAL_ERROR "private header installed: ${private_header}")
    endif()
endforeach()

file(
    WRITE "${consumer}"
    "#include <core/socket/stream/SocketServer.h>\n#include <core/socket/stream/SocketClient.h>\n#include <net/in/stream/legacy/SocketServer.h>\n#include <net/in/stream/legacy/SocketClient.h>\n#include <express/legacy/in/Server.h>\nint main() { return 0; }\n"
)
set(exe "${stage}/consumer")
set(
    consumer_compile_command
    "${CMAKE_CXX_COMPILER}"
    -std=c++20
    "${consumer}"
    "-I${include_directory}"
    "-L${library_directory}"
    "-L${http_library_directory}"
    "-Wl,-rpath,${library_directory}"
    "-Wl,-rpath,${http_library_directory}"
    -lsnodec-core
    -lsnodec-core-socket
    -lsnodec-core-socket-stream
    -lsnodec-net
    -lsnodec-net-in
    -lsnodec-net-in-phy
    -lsnodec-net-in-phy-stream
    -lsnodec-net-in-stream
    -lsnodec-core-socket-stream-legacy
    -lsnodec-net-in-stream-legacy
    -lsnodec-http
    -lsnodec-http-server
    -lsnodec-http-server-express
    -lsnodec-http-server-express-legacy-in
    -o
    "${exe}"
)
execute_process(
    COMMAND ${consumer_compile_command}
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_output
    ERROR_VARIABLE compile_error
)
string(JOIN " " consumer_compile_command_text ${consumer_compile_command})
message(STATUS "Installed consumer compile command: ${consumer_compile_command_text}")
if(NOT compile_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed consumer compile failed\n${compile_output}\n${compile_error}"
    )
endif()
execute_process(
    COMMAND "${exe}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_output
    ERROR_VARIABLE run_error
)
if(NOT run_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed consumer execution failed\n${run_output}\n${run_error}"
    )
endif()

set(
    snodec_config_file
    "${library_directory}/cmake/snodec/snodecConfig.cmake"
)
if(NOT EXISTS "${snodec_config_file}")
    message(
        FATAL_ERROR
            "installed snodecConfig.cmake not found: ${snodec_config_file}"
    )
endif()
get_filename_component(snodec_config_dir "${snodec_config_file}" DIRECTORY)

set(generic_consumer_source "${stage}/generic-cmake-consumer")
set(generic_consumer_build "${stage}/generic-cmake-consumer-build")
file(MAKE_DIRECTORY "${generic_consumer_source}")
file(
    WRITE "${generic_consumer_source}/main.cpp"
    "#include <core/socket/stream/SocketClient.h>\n#include <net/un/stream/legacy/SocketClient.h>\nint main() { return 0; }\n"
)
file(
    WRITE "${generic_consumer_source}/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.18)\n"
    "project(SNodeCInstalledConsumer LANGUAGES CXX)\n"
    "find_package(snodec CONFIG REQUIRED COMPONENTS core net-un-stream-legacy)\n"
    "add_executable(generic-consumer main.cpp)\n"
    "target_compile_features(generic-consumer PRIVATE cxx_std_20)\n"
    "target_link_libraries(generic-consumer PRIVATE snodec::core snodec::net-un-stream-legacy)\n"
)
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -S "${generic_consumer_source}" -B
        "${generic_consumer_build}" "-Dsnodec_DIR=${snodec_config_dir}"
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
        -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE
        -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=TRUE
        -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE
        -DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=TRUE
    RESULT_VARIABLE generic_configure_result
    OUTPUT_VARIABLE generic_configure_output
    ERROR_VARIABLE generic_configure_error
)
if(NOT generic_configure_result EQUAL 0)
    message(
        FATAL_ERROR
            "generic installed CMake consumer configure failed\n${generic_configure_output}\n${generic_configure_error}"
    )
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${generic_consumer_build}"
    RESULT_VARIABLE generic_build_result
    OUTPUT_VARIABLE generic_build_output
    ERROR_VARIABLE generic_build_error
)
if(NOT generic_build_result EQUAL 0)
    message(
        FATAL_ERROR
            "generic installed CMake consumer build failed\n${generic_build_output}\n${generic_build_error}"
    )
endif()
execute_process(
    COMMAND "${generic_consumer_build}/generic-consumer"
    RESULT_VARIABLE generic_run_result
    OUTPUT_VARIABLE generic_run_output
    ERROR_VARIABLE generic_run_error
)
if(NOT generic_run_result EQUAL 0)
    message(
        FATAL_ERROR
            "generic installed CMake consumer execution failed\n${generic_run_output}\n${generic_run_error}"
    )
endif()
