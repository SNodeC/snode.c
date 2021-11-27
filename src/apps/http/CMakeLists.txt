cmake_minimum_required(VERSION 3.0)

get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR} NAME)

set(IN 1)
set(IN6 2)
set(L2 3)
set(RF 4)
set(UN 5)

set(LEGACY 1)
set(TLS 2)

add_compile_definitions(
    IN=${IN}
    IN6=${IN6}
    L2=${L2}
    RF=${RF}
    UN=${UN}
    LEGACY=${LEGACY}
    TLS=${TLS}
)

# set(NETS "in" "in6" "l2" "rf" "un")
set(NETS "in" "in6")
if(BLUEZ_FOUND)
    list(APPEND NETS "rf")
endif(BLUEZ_FOUND)

foreach(NET IN LISTS NETS)
    if(${NET} STREQUAL "in")
        set(NET_TYPE ${IN})
    elseif(${NET} STREQUAL "in6")
        set(NET_TYPE ${IN6})
    elseif(${NET} STREQUAL "l2")
        set(NET_TYPE ${L2})
    elseif(${NET} STREQUAL "rf")
        set(NET_TYPE ${RF})
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

        add_executable(${NET}${STREAM}httpserver httpserver.cpp)
        target_link_libraries(${NET}${STREAM}httpserver PRIVATE snodec::express)
        target_compile_definitions(
            ${NET}${STREAM}httpserver
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS ${NET}${STREAM}httpserver
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        add_executable(
            ${NET}${STREAM}httpserverclientcert httpserverclientcert.cpp
        )
        target_link_libraries(
            ${NET}${STREAM}httpserverclientcert PRIVATE snodec::express
        )
        target_compile_definitions(
            ${NET}${STREAM}httpserverclientcert
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS ${NET}${STREAM}httpserverclientcert
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        add_executable(${NET}${STREAM}httpclient httpclient.cpp)
        target_link_libraries(
            ${NET}${STREAM}httpclient PRIVATE snodec::httpclient
        )
        target_compile_definitions(
            ${NET}${STREAM}httpclient
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS ${NET}${STREAM}httpclient
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        add_executable(
            ${NET}${STREAM}httpclientclientcert httpclientclientcert.cpp
        )
        target_link_libraries(
            ${NET}${STREAM}httpclientclientcert PRIVATE snodec::httpclient
        )
        target_compile_definitions(
            ${NET}${STREAM}httpclientclientcert
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS ${NET}${STREAM}httpclientclientcert
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

    endforeach()
endforeach()
