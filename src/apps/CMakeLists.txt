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
target_link_libraries(simpleserver6 PRIVATE snodec::express)
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
target_link_libraries(jsonclient PRIVATE snodec::httpclient)
install(TARGETS jsonclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testparser testparsers.cpp)
target_link_libraries(testparser PRIVATE snodec::httpserver snodec::httpclient)
install(TARGETS testparser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(client4 client4.cpp)
target_link_libraries(client4 PRIVATE snodec::httpclient)
install(TARGETS client4 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(client6 client6.cpp)
target_link_libraries(client6 PRIVATE snodec::httpclient)
install(TARGETS client6 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(client6clientcert client6clientcert.cpp)
target_link_libraries(client6clientcert PRIVATE snodec::httpclient)
install(TARGETS client6clientcert RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(lowlevelclient lowlevelclient.cpp)
target_link_libraries(lowlevelclient PRIVATE snodec::httpclient)
install(TARGETS lowlevelclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testregex testregex.cpp)
target_link_libraries(testregex PRIVATE snodec::express)
install(TARGETS testregex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(warema-jalousien warema-jalousien.cpp)
target_link_libraries(warema-jalousien PRIVATE snodec::express)
install(TARGETS warema-jalousien RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(BLUEZ_FOUND)

    add_executable(simpleserverrfcomm simpleserverrfcomm.cpp)
    target_link_libraries(
        simpleserverrfcomm PRIVATE snodec::express snodec::net-rf
    )
    install(TARGETS simpleserverrfcomm
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    add_executable(clientrfcomm clientrfcomm.cpp)
    target_link_libraries(
        clientrfcomm PRIVATE snodec::httpclient snodec::net-rf
    )
    install(TARGETS clientrfcomm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

endif(BLUEZ_FOUND)

add_executable(vhostserver vhostserver.cpp)
target_link_libraries(vhostserver PRIVATE snodec::express)
install(TARGETS vhostserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testpipe testpipe.cpp)
target_link_libraries(testpipe PRIVATE snodec::core)
install(TARGETS testpipe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(websocket)
add_subdirectory(all)
add_subdirectory(rf)
add_subdirectory(l2)
add_subdirectory(in)
add_subdirectory(in6)
add_subdirectory(un)
