cmake_minimum_required(VERSION 3.0)

add_library(echoserver SHARED Echo.cpp EchoFactory.cpp)

target_include_directories(echoserver PUBLIC ${PROJECT_SOURCE_DIR})

set_target_properties(
    echoserver
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${WEBSOCKET_SERVER_LIBDIR}"
               OUTPUT_NAME "snodec-websocket-echo"
               SOVERSION 1 # PREFIX "ssp"
)

install(TARGETS echoserver LIBRARY DESTINATION lib/snode.c/web/websocket/server)

if(CHECK_INCLUDES)
    set_property(
        TARGET echoserver PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                   ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
