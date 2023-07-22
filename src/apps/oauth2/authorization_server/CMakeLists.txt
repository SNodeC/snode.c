cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(authorization_server AuthorizationServer.cpp)

target_link_libraries(
    authorization_server
    PRIVATE snodec::http-server-express snodec::net-in-stream-legacy
            snodec::db-mariadb
)

if(SNODEC_INTREE_BUILD)

    set_target_properties(
        authorization_server
        PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${HTTP_SERVER_RPATH}"
    )

endif(SNODEC_INTREE_BUILD)

install(TARGETS authorization_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                             COMPONENT apps
)
