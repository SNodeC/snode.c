cmake_minimum_required(VERSION 3.0)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR snodec::websocket-server
    WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS snodec::websocket-server
    WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR snodec::websocket-server
    WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR
)

set(ECHOSERVERSUBPROTOCOL_CPP Echo.cpp EchoFactory.cpp)

set(ECHOSERVERSUBPROTOCOL_H Echo.h EchoFactory.h)

add_library(
    echoserversubprotocol SHARED ${ECHOSERVERSUBPROTOCOL_CPP}
                                 ${ECHOSERVERSUBPROTOCOL_H}
)

target_include_directories(
    echoserversubprotocol PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

set_target_properties(
    echoserversubprotocol
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR}"
               OUTPUT_NAME "snodec-websocket-echo"
               SOVERSION 1 # PREFIX "ssp"
)

install(TARGETS echoserversubprotocol
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR}
)

if(CHECK_INCLUDES)
    set_property(
        TARGET echoserversubprotocol PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                              ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
