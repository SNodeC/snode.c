cmake_minimum_required(VERSION 3.0)

get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR} NAME)

set(IN 1)
set(IN6 2)
set(L2 3)
set(RC 4)
set(UN 5)

set(LEGACY 1)
set(TLS 2)

add_compile_definitions(
    IN=${IN}
    IN6=${IN6}
    L2=${L2}
    RC=${RC}
    UN=${UN}
    LEGACY=${LEGACY}
    TLS=${TLS}
)

set(HTTPCLIENT_CPP httpclient.cpp)
set(HTTPCLIENT_H model/clients.h)

set(HTTPCLIENTCLIENTCERT_CPP httpclientclientcert.cpp)
set(HTTPCLIENTCLIENTCERT_H model/clients.h)

set(HTTPSERVER_CPP httpserver.cpp)
set(HTTPSERVER_H model/servers.h)

set(HTTPSERVERCLIENTCERT_CPP httpserverclientcert.cpp)
set(HTTPSERVERCLIENTCERT_H model/servers.h)

set(NETS "in" "in6" "un")
if(BLUEZ_FOUND)
    list(APPEND NETS "rc")
endif(BLUEZ_FOUND)

foreach(NET IN LISTS NETS)
    if(${NET} STREQUAL "in")
        set(NET_TYPE ${IN})
    elseif(${NET} STREQUAL "in6")
        set(NET_TYPE ${IN6})
    elseif(${NET} STREQUAL "l2")
        set(NET_TYPE ${L2})
    elseif(${NET} STREQUAL "rc")
        set(NET_TYPE ${RC})
    elseif(${NET} STREQUAL "un")
        set(NET_TYPE ${UN})
    endif()

    set(STREAMS "legacy" "tls")
    foreach(STREAM IN LISTS STREAMS)
        if(${STREAM} STREQUAL "legacy")
            set(STREAM_TYPE ${LEGACY})
        elseif(${STREAM} STREQUAL "tls")
            set(STREAM_TYPE ${TLS})
        endif()

        add_executable(
            httpserver-${STREAM}-${NET} ${HTTPSERVER_CPP} ${HTTPSERVER_H}
        )
        target_link_libraries(
            httpserver-${STREAM}-${NET}
            PUBLIC snodec::express snodec::core-${STREAM} snodec::net-${NET}
        )
        target_compile_definitions(
            httpserver-${STREAM}-${NET}
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS httpserver-${STREAM}-${NET}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        add_executable(
            httpclient-${STREAM}-${NET} ${HTTPCLIENT_CPP} ${HTTPCLIENT_H}
        )
        target_link_libraries(
            httpclient-${STREAM}-${NET}
            PUBLIC snodec::httpclient snodec::core-${STREAM} snodec::net-${NET}
        )
        target_compile_definitions(
            httpclient-${STREAM}-${NET}
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS httpclient-${STREAM}-${NET}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        if(${STREAM} STREQUAL "tls")
            add_executable(
                httpserverclientcert-${STREAM}-${NET}
                ${HTTPSERVERCLIENTCERT_CPP} ${HTTPSERVERCLIENTCERT_H}
            )
            target_link_libraries(
                httpserverclientcert-${STREAM}-${NET}
                PUBLIC snodec::express snodec::core-${STREAM}
                       snodec::net-${NET}
            )
            target_compile_definitions(
                httpserverclientcert-${STREAM}-${NET}
                PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                        STREAM_TYPE=${STREAM_TYPE}
            )
            install(TARGETS httpserverclientcert-${STREAM}-${NET}
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )

            add_executable(
                httpclientclientcert-${STREAM}-${NET}
                ${HTTPCLIENTCLIENTCERT_CPP} ${HTTPCLIENTCLIENTCERT_H}
            )
            target_link_libraries(
                httpclientclientcert-${STREAM}-${NET}
                PUBLIC snodec::httpclient snodec::core-${STREAM}
                       snodec::net-${NET}
            )
            target_compile_definitions(
                httpclientclientcert-${STREAM}-${NET}
                PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                        STREAM_TYPE=${STREAM_TYPE}
            )
            install(TARGETS httpclientclientcert-${STREAM}-${NET}
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
    endforeach()
endforeach()
