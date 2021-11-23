cmake_minimum_required(VERSION 3.0)

add_subdirectory(model)

get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR} NAME)

set(NET ${CURRENT_DIR})

add_executable(${NET}web${TYPE}client webclient.cpp)
target_link_libraries(${NET}web${TYPE}client PRIVATE snodec::httpclient)
target_compile_definitions(${NET}webclient PRIVATE TYPE=legacy)
install(TARGETS ${NET}webclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

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

set(NETS "in" "in6" "l2" "rf" "un")
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

        add_executable(${NET}${STREAM}echoclient echoclient.cpp)
        target_link_libraries(
            ${NET}${STREAM}echoclient PRIVATE snodec::net-${NET}
                                              echosocketcontext
        )
        target_compile_definitions(
            ${NET}${STREAM}echoclient
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS ${NET}${STREAM}echoclient
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        add_executable(${NET}${STREAM}echoserver echoserver.cpp)
        target_link_libraries(
            ${NET}${STREAM}echoserver PRIVATE snodec::net-${NET}
                                              echosocketcontext
        )
        target_compile_definitions(
            ${NET}${STREAM}echoserver
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS ${NET}${STREAM}echoserver
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

    endforeach()
endforeach()
