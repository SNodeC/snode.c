cmake_minimum_required(VERSION 3.0)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR
)

set(ECHOCLIENTSUBPROTOCOL_CPP Echo.cpp EchoFactory.cpp)

set(ECHOCLIENTSUBPROTOCOL_H Echo.h EchoFactory.h)

add_library(
    echoclientsubprotocol SHARED ${ECHOCLIENTSUBPROTOCOL_CPP}
                                 ${ECHOCLIENTSUBPROTOCOL_H}
)

target_include_directories(
    echoclientsubprotocol PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

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
