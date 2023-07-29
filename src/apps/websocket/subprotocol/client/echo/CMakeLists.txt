cmake_minimum_required(VERSION 3.18)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
)

get_target_property(
    HTTP_UPGRADE_INSTALL_LIBDIR snodec::http-client HTTP_UPGRADE_INSTALL_LIBDIR
)

set(ECHOCLIENTSUBPROTOCOL_CPP Echo.cpp EchoFactory.cpp)

set(ECHOCLIENTSUBPROTOCOL_H Echo.h EchoFactory.h)

add_library(
    websocket-echo-client SHARED ${ECHOCLIENTSUBPROTOCOL_CPP}
                                 ${ECHOCLIENTSUBPROTOCOL_H}
)

target_include_directories(
    websocket-echo-client PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

target_link_libraries(websocket-echo-client PUBLIC snodec::websocket-client)

set_target_properties(
    websocket-echo-client
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}"
               OUTPUT_NAME "snodec-websocket-echo-client"
               SOVERSION ${SNODEC_SOVERSION} # PREFIX "ssp"
               INSTALL_RPATH
               "${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR}"
)

install(TARGETS websocket-echo-client
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}
                COMPONENT apps
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocket-echo-client PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                              ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
