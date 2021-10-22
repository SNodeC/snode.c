cmake_minimum_required(VERSION 3.0)

add_library(echoclient SHARED Echo.cpp EchoFactory.cpp)

target_include_directories(echoclient PUBLIC ${PROJECT_SOURCE_DIR})

set_target_properties(
    echoclient
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${WEBSOCKET_CLIENT_LIBDIR}"
               OUTPUT_NAME "snodec-websocket-echo"
               SOVERSION 1 # PREFIX "ssp"
)

install(TARGETS echoclient
        LIBRARY DESTINATION lib/snode.c/web/websocket/client
)

if(CHECK_INCLUDES)
    set_property(
        TARGET echoclient PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                   ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
