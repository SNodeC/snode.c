cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(client_app ClientApp.cpp)

target_link_libraries(
    client_app PRIVATE snodec::http-server-express snodec::net-in-stream-legacy
)

if(SNODEC_INTREE_BUILD)

    set_target_properties(
        client_app PROPERTIES INSTALL_RPATH "${HTTP_SERVER_RPATH}"
    )

endif(SNODEC_INTREE_BUILD)

install(TARGETS client_app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)
