cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(snode.c main.cpp)
target_link_libraries(snode.c PRIVATE snodec::express)
install(TARGETS snode.c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testpost testpost.cpp)
target_link_libraries(testpost PRIVATE snodec::express)
install(TARGETS testpost RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(simpleserver simpleserver.cpp)
target_link_libraries(simpleserver PRIVATE snodec::express)
install(TARGETS simpleserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(simpleserver6 simpleserver6.cpp)
target_link_libraries(simpleserver6 PRIVATE express)
install(TARGETS simpleserver6 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(simpleserver6clientcert simpleserver6clientcert.cpp)
target_link_libraries(simpleserver6clientcert PRIVATE snodec::express)
install(TARGETS simpleserver6clientcert
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(verysimpleserver verysimpleserver.cpp)
target_link_libraries(verysimpleserver PRIVATE snodec::express)
install(TARGETS verysimpleserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(NLOHMANN_JSON_FOUND)
    add_executable(jsonserver jsonserver.cpp)
    target_link_libraries(jsonserver PRIVATE snodec::express)
    install(TARGETS jsonserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(NLOHMANN_JSON_FOUND)

add_executable(jsonclient jsonclient.cpp)
target_link_libraries(jsonclient PRIVATE snodec::express)
install(TARGETS jsonclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testparser testparsers.cpp)
target_link_libraries(testparser PRIVATE snodec::express)
install(TARGETS testparser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(client4 client4.cpp)
target_link_libraries(client4 PRIVATE snodec::http)
install(TARGETS client4 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(client6 client6.cpp)
target_link_libraries(client6 PRIVATE snodec::http)
install(TARGETS client6 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(client6clientcert client6clientcert.cpp)
target_link_libraries(client6clientcert PRIVATE snodec::http)
install(TARGETS client6clientcert RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(lowlevelclient lowlevelclient.cpp)
target_link_libraries(lowlevelclient PRIVATE snodec::http)
install(TARGETS lowlevelclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testregex testregex.cpp)
target_link_libraries(testregex PRIVATE snodec::express)
install(TARGETS testregex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(warema-jalousien warema-jalousien.cpp)
target_link_libraries(warema-jalousien PRIVATE snodec::express)
install(TARGETS warema-jalousien RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BLUEZ_FOUND)
    add_executable(btlowlevelclient btlowlevelclient.cpp)
    target_link_libraries(btlowlevelclient PRIVATE snodec::net-bt)
    install(TARGETS btlowlevelclient
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    add_executable(btlowlevelserver btlowlevelserver.cpp)
    target_link_libraries(btlowlevelserver PRIVATE snodec::net-bt)
    install(TARGETS btlowlevelserver
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    add_executable(btlowleveltlsclient btlowleveltlsclient.cpp)
    target_link_libraries(btlowleveltlsclient PRIVATE snodec::net-bt)
    install(TARGETS btlowleveltlsclient
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    add_executable(btlowleveltlsserver btlowleveltlsserver.cpp)
    target_link_libraries(btlowleveltlsserver PRIVATE snodec::net-bt)
    install(TARGETS btlowleveltlsserver
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    add_executable(l2lowlevelclient l2lowlevelclient.cpp)
    target_link_libraries(l2lowlevelclient PRIVATE snodec::net-bt)
    install(TARGETS l2lowlevelclient
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    add_executable(l2lowlevelserver l2lowlevelserver.cpp)
    target_link_libraries(l2lowlevelserver PRIVATE snodec::net-bt)
    install(TARGETS l2lowlevelserver
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif(BLUEZ_FOUND)

add_executable(vhostserver vhostserver.cpp)
target_link_libraries(vhostserver PRIVATE snodec::express)
install(TARGETS vhostserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testpipe testpipe.cpp)
target_link_libraries(testpipe PRIVATE snodec::net)
install(TARGETS testpipe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(websocketparser websocketparser.cpp)
target_link_libraries(websocketparser PRIVATE snodec::express)
install(TARGETS websocketparser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(websockettestclient websocketclient.cpp)
target_link_libraries(websockettestclient PRIVATE snodec::http)
install(TARGETS websockettestclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
