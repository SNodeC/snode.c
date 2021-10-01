cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(snode.c main.cpp)
target_link_libraries(snode.c PRIVATE express)
install(TARGETS snode.c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    snode.c
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(testpost testpost.cpp)
target_link_libraries(testpost PRIVATE express)
install(TARGETS testpost RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    testpost
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(simpleserver simpleserver.cpp)
target_link_libraries(simpleserver PRIVATE express)
install(TARGETS simpleserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    simpleserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(simpleserver6 simpleserver6.cpp)
target_link_libraries(simpleserver6 PRIVATE express)
install(TARGETS simpleserver6 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    simpleserver6
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(simpleserver6clientcert simpleserver6clientcert.cpp)
target_link_libraries(simpleserver6clientcert PRIVATE express)
install(TARGETS simpleserver6clientcert RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    simpleserver6clientcert
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(verysimpleserver verysimpleserver.cpp)
target_link_libraries(verysimpleserver PRIVATE express)
install(TARGETS verysimpleserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    verysimpleserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(jsonserver jsonserver.cpp)
target_link_libraries(jsonserver PRIVATE express)
install(TARGETS jsonserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    jsonserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(jsonclient jsonclient.cpp)
target_link_libraries(jsonclient PRIVATE express)
install(TARGETS jsonclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    jsonclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
add_executable(testparser testparsers.cpp)
target_link_libraries(testparser PRIVATE express)
install(TARGETS testparser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    testparser
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(client4 client4.cpp)
target_link_libraries(client4 PRIVATE web)
install(TARGETS client4 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    client4
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(client6 client6.cpp)
target_link_libraries(client6 PRIVATE web)
install(TARGETS client6 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    client6
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(client6clientcert client6clientcert.cpp)
target_link_libraries(client6clientcert PRIVATE web)
install(TARGETS client6clientcert RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    client6clientcert
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(lowlevelclient lowlevelclient.cpp)
target_link_libraries(lowlevelclient PRIVATE web)
install(TARGETS lowlevelclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    lowlevelclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(testregex testregex.cpp)
target_link_libraries(testregex PRIVATE express)
install(TARGETS testregex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    testregex
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(warema-jalousien warema-jalousien.cpp)
target_link_libraries(warema-jalousien PRIVATE express)
install(TARGETS warema-jalousien RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    warema-jalousien
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(btlowlevelclient btlowlevelclient.cpp)
target_link_libraries(btlowlevelclient PRIVATE web)
install(TARGETS btlowlevelclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    btlowlevelclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(btlowlevelserver btlowlevelserver.cpp)
target_link_libraries(btlowlevelserver PRIVATE net)
install(TARGETS btlowlevelserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    btlowlevelserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(btlowleveltlsclient btlowleveltlsclient.cpp)
target_link_libraries(btlowleveltlsclient PRIVATE web)
install(TARGETS btlowleveltlsclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    btlowleveltlsclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(btlowleveltlsserver btlowleveltlsserver.cpp)
target_link_libraries(btlowleveltlsserver PRIVATE net)
install(TARGETS btlowleveltlsserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    btlowleveltlsserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(l2lowlevelclient l2lowlevelclient.cpp)
target_link_libraries(l2lowlevelclient PRIVATE web)
install(TARGETS l2lowlevelclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    l2lowlevelclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(l2lowlevelserver l2lowlevelserver.cpp)
target_link_libraries(l2lowlevelserver PRIVATE net)
install(TARGETS l2lowlevelserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    l2lowlevelserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(vhostserver vhostserver.cpp)
target_link_libraries(vhostserver PRIVATE express)
install(TARGETS vhostserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    vhostserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(testpipe testpipe.cpp)
target_link_libraries(testpipe PRIVATE net)
install(TARGETS testpipe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    testpipe
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(websocketparser websocketparser.cpp)
target_link_libraries(websocketparser PRIVATE express)
install(TARGETS websocketparser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    websocketparser
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)

add_executable(websocketclient websocketclient.cpp)
target_link_libraries(websocketclient PRIVATE web)
install(TARGETS websocketclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(
    websocketclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
