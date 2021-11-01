cmake_minimum_required(VERSION 3.0)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR snodec::websocketclient
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS snodec::websocketclient
    WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR snodec::websocketclient
    WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR
)

add_library(echoclientsubprotocol SHARED Echo.cpp EchoFactory.cpp)

target_include_directories(echoclientsubprotocol PUBLIC ${PROJECT_SOURCE_DIR})

set_target_properties(
    echoclientsubprotocol
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR}"
               OUTPUT_NAME "snodec-websocket-echo"
               SOVERSION 1 # PREFIX "ssp"
)

install(TARGETS echoclientsubprotocol
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}
)

if(CHECK_INCLUDES)
    set_property(
        TARGET echoclientsubprotocol PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                              ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
