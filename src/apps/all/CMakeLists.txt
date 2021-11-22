cmake_minimum_required(VERSION 3.0)

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
        set(NETI ${IN})
    elseif(${NET} STREQUAL "in6")
        set(NETI ${IN6})
    elseif(${NET} STREQUAL "l2")
        set(NETI ${L2})
    elseif(${NET} STREQUAL "rf")
        set(NETI ${RF})
    elseif(${NET} STREQUAL "un")
        set(NETI ${UN})
    endif()

    set(TYPES "legacy" "tls")
    foreach(TYPE IN LISTS TYPES)
        if(${TYPE} STREQUAL "legacy")
            set(TYPEI ${LEGACY})
        elseif(${TYPE} STREQUAL "tls")
            set(TYPEI ${TLS})
        endif()

        add_executable(${NET}${TYPE}echoclient echoclient.cpp)
        target_link_libraries(
            ${NET}${TYPE}echoclient PRIVATE snodec::net-${NET}
        )
        target_compile_definitions(
            ${NET}${TYPE}echoclient PRIVATE NET=${NET} TYPE=${TYPE}
                                                NETI=${NETI} TYPEI=${TYPEI}
        )
        install(TARGETS ${NET}${TYPE}echoclient
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        add_executable(${NET}${TYPE}echoserver echoserver.cpp)
        target_link_libraries(
            ${NET}${TYPE}echoserver PRIVATE snodec::net-${NET}
        )
        target_compile_definitions(
            ${NET}${TYPE}echoserver PRIVATE NET=${NET} TYPE=${TYPE}
                                                NETI=${NETI} TYPEI=${TYPEI}
        )
        install(TARGETS ${NET}${TYPE}echoserver
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

    endforeach()
endforeach()
