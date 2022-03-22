cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(snode.c main.cpp)
target_link_libraries(
    snode.c PRIVATE snodec::express snodec::core snodec::net-in
)
install(TARGETS snode.c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testpost testpost.cpp)
target_link_libraries(
    testpost PRIVATE snodec::express snodec::core-legacy snodec::core-tls
                     snodec::net-in
)
install(TARGETS testpost RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(verysimpleserver verysimpleserver.cpp)
target_link_libraries(
    verysimpleserver PRIVATE snodec::express snodec::core-legacy
                             snodec::core-tls snodec::net-in
)
install(TARGETS verysimpleserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(NLOHMANN_JSON_FOUND)
    add_executable(jsonserver jsonserver.cpp)
    target_link_libraries(
        jsonserver PRIVATE snodec::express snodec::core-legacy snodec::net-in
    )
    install(TARGETS jsonserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(NLOHMANN_JSON_FOUND)

add_executable(jsonclient jsonclient.cpp)
target_link_libraries(
    jsonclient PRIVATE snodec::httpclient snodec::core-legacy snodec::net-in
)
install(TARGETS jsonclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testparser testparsers.cpp)
target_link_libraries(
    testparser PRIVATE snodec::httpserver snodec::httpclient snodec::core
                       snodec::logger
)
install(TARGETS testparser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(lowlevelclient lowlevelclient.cpp)
target_link_libraries(
    lowlevelclient PRIVATE snodec::httpclient snodec::core-legacy
                           snodec::core-tls snodec::net-in
)
install(TARGETS lowlevelclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testregex testregex.cpp)
target_link_libraries(
    testregex PRIVATE snodec::express snodec::core-legacy snodec::core-tls
                      snodec::net-in
)
install(TARGETS testregex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(warema-jalousien warema-jalousien.cpp)
target_link_libraries(
    warema-jalousien PRIVATE snodec::express snodec::core-legacy snodec::net-in
)
install(TARGETS warema-jalousien RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(vhostserver vhostserver.cpp)
target_link_libraries(
    vhostserver PRIVATE snodec::express snodec::core-legacy snodec::core-tls
                        snodec::net-in6
)
install(TARGETS vhostserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(testpipe testpipe.cpp)
target_link_libraries(testpipe PRIVATE snodec::core)
install(TARGETS testpipe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(websocket)
add_subdirectory(echo)
add_subdirectory(http)
add_subdirectory(rc)
add_subdirectory(l2)
add_subdirectory(in)
add_subdirectory(in6)
add_subdirectory(un)
add_subdirectory(towercalculator)
