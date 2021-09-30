cmake_minimum_required(VERSION 3.0)

add_library(echo SHARED Echo.cpp EchoFactory.cpp)

target_include_directories(echo PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(echo snodec::websocketserver)

set_target_properties(
    echo
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

set_target_properties(
    echo PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${WEBSOCKET_SERVER_LIBDIR}"
                    # PREFIX "ssp"
)

install(TARGETS echo
        LIBRARY DESTINATION lib/snode.c/web/websocket/subprotocol/server
)

if(CHECK_INCLUDES)
    set_property(
        TARGET echo PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
