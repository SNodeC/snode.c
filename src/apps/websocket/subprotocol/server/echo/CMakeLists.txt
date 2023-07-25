cmake_minimum_required(VERSION 3.18)

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

get_target_property(
    HTTP_SERVER_UPGRADE_INSTALL_LIBDIR snodec::http-server
    HTTP_SERVER_UPGRADE_INSTALL_LIBDIR
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

target_link_libraries(echoserversubprotocol PUBLIC snodec::websocket-server)

set_target_properties(
    echoserversubprotocol
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR}"
               OUTPUT_NAME "snodec-websocket-echo"
               SOVERSION ${SNODEC_SOVERSION} # PREFIX "ssp"
               INSTALL_RPATH
               "${CMAKE_INSTALL_PREFIX}/${HTTP_SERVER_UPGRADE_INSTALL_LIBDIR}"
)

install(TARGETS echoserversubprotocol
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR}
                COMPONENT apps
)

if(CHECK_INCLUDES)
    set_property(
        TARGET echoserversubprotocol PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                              ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
