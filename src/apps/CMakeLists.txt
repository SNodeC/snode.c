cmake_minimum_required(VERSION 3.0)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-missing-prototypes)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

get_target_property(HTTP_SERVER_RPATH snodec::http-server HTTP_SERVER_RPATH)
get_target_property(HTTP_CLIENT_RPATH snodec::http-client HTTP_CLIENT_RPATH)

add_executable(snode.c main.cpp)
target_link_libraries(snode.c PUBLIC snodec::core)
install(TARGETS snode.c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                COMPONENT apps
)

add_executable(testpost testpost.cpp)
target_link_libraries(
    testpost PRIVATE snodec::http-server-express snodec::net-in-stream-legacy
                     snodec::net-in-stream-tls
)
set_target_properties(testpost PROPERTIES INSTALL_RPATH "${HTTP_SERVER_RPATH}")
install(TARGETS testpost RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                 COMPONENT apps
)

add_executable(verysimpleserver verysimpleserver.cpp)
target_link_libraries(
    verysimpleserver
    PRIVATE snodec::http-server-express snodec::net-in-stream-legacy
            snodec::net-in-stream-tls
)
set_target_properties(
    verysimpleserver PROPERTIES INSTALL_RPATH "${HTTP_SERVER_RPATH}"
)
install(TARGETS verysimpleserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                         COMPONENT apps
)

if(NLOHMANN_JSON_FOUND)
    add_executable(jsonserver jsonserver.cpp)
    target_link_libraries(
        jsonserver PRIVATE snodec::http-server-express
                           snodec::net-in-stream-legacy
    )
    set_target_properties(
        jsonserver PROPERTIES INSTALL_RPATH "${HTTP_SERVER_RPATH}"
    )
    install(TARGETS jsonserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                       COMPONENT apps
    )
endif(NLOHMANN_JSON_FOUND)

add_executable(jsonclient jsonclient.cpp)
target_link_libraries(
    jsonclient PRIVATE snodec::http-client snodec::net-in-stream-legacy
)
set_target_properties(
    jsonclient PROPERTIES INSTALL_RPATH "${HTTP_CLIENT_RPATH}"
)
install(TARGETS jsonclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)

add_executable(testparser testparsers.cpp)
target_link_libraries(
    testparser PRIVATE snodec::http-server snodec::http-client snodec::core
                       snodec::core-socket-stream snodec::logger
)
set_target_properties(
    testparser PROPERTIES INSTALL_RPATH
                          "${HTTP_SERVER_RPATH};${HTTP_CLIENT_RPATH}"
)
install(TARGETS testparser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)

if(MARIADB_FOUND)
    add_executable(testregex testregex.cpp)
    target_link_libraries(
        testregex
        PRIVATE snodec::http-server-express snodec::net-in-stream-legacy
                snodec::net-in-stream-tls snodec::db-mariadb
    )
    set_target_properties(
        testregex PROPERTIES INSTALL_RPATH "${HTTP_SERVER_RPATH}"
    )
    install(TARGETS testregex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                      COMPONENT apps
    )
endif(MARIADB_FOUND)

add_executable(warema-jalousien warema-jalousien.cpp)
target_link_libraries(
    warema-jalousien PRIVATE snodec::http-server-express
                             snodec::net-in-stream-legacy
)
set_target_properties(
    warema-jalousien PROPERTIES INSTALL_RPATH "${HTTP_SERVER_RPATH}"
)
install(TARGETS warema-jalousien RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                         COMPONENT apps
)

add_executable(vhostserver vhostserver.cpp)
target_link_libraries(
    vhostserver
    PRIVATE snodec::http-server-express snodec::net-in6-stream-legacy
            snodec::net-in6-stream-tls
)
set_target_properties(
    vhostserver PROPERTIES INSTALL_RPATH "${HTTP_SERVER_RPATH}"
)
install(TARGETS vhostserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                    COMPONENT apps
)

add_executable(testpipe testpipe.cpp)
target_link_libraries(testpipe PRIVATE snodec::core)
install(TARGETS testpipe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                 COMPONENT apps
)

add_executable(configtest configtest.cpp)
target_link_libraries(configtest PRIVATE snodec::core)
install(TARGETS configtest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)

add_executable(testbasicauthentication testbasicauthentication.cpp)
target_link_libraries(
    testbasicauthentication
    PRIVATE snodec::http-server-express snodec::net-in6-stream-legacy
            snodec::net-in6-stream-tls
)
set_target_properties(
    testbasicauthentication PROPERTIES INSTALL_RPATH "${HTTP_SERVER_RPATH}"
)
install(TARGETS testbasicauthentication
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
)

macro(subdirlist result curdir)
    file(
        GLOB children
        RELATIVE ${curdir}
        ${curdir}/*
    )
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child}
           AND EXISTS ${curdir}/${child}/CMakeLists.txt
        )
            list(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

subdirlist(subdirs ${CMAKE_CURRENT_SOURCE_DIR})

foreach(subdir ${subdirs})
    add_subdirectory(${subdir})
endforeach()
