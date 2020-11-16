cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(snode.c main.cpp)
target_compile_options(snode.c PRIVATE --warn-all)
target_link_libraries(snode.c LINK_PUBLIC express)
install(TARGETS snode.c RUNTIME DESTINATION bin)
set_target_properties(
    snode.c
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET snode.c PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(testpost testpost.cpp)
target_compile_options(testpost PRIVATE --warn-all)
target_link_libraries(testpost LINK_PUBLIC express)
install(TARGETS testpost RUNTIME DESTINATION bin)
set_target_properties(
    testpost
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET testpost PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                 ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(simpleserver simpleserver.cpp)
target_compile_options(simpleserver PRIVATE --warn-all)
target_link_libraries(simpleserver LINK_PUBLIC express)
install(TARGETS simpleserver RUNTIME DESTINATION bin)
set_target_properties(
    simpleserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET simpleserver PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                     ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(simpleserver6 simpleserver6.cpp)
target_compile_options(simpleserver6 PRIVATE --warn-all)
target_link_libraries(simpleserver6 LINK_PUBLIC express)
install(TARGETS simpleserver6 RUNTIME DESTINATION bin)
set_target_properties(
    simpleserver6
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET simpleserver6 PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                      ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(simpleserver6clientcert simpleserver6clientcert.cpp)
target_compile_options(simpleserver6clientcert PRIVATE --warn-all)
target_link_libraries(simpleserver6clientcert LINK_PUBLIC express)
install(TARGETS simpleserver6clientcert RUNTIME DESTINATION bin)
set_target_properties(
    simpleserver6clientcert
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET simpleserver6clientcert PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                                ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(verysimpleserver verysimpleserver.cpp)
target_compile_options(verysimpleserver PRIVATE --warn-all)
target_link_libraries(verysimpleserver LINK_PUBLIC express)
install(TARGETS verysimpleserver RUNTIME DESTINATION bin)
set_target_properties(
    verysimpleserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET verysimpleserver PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                         ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(jsonserver jsonserver.cpp)
target_compile_options(jsonserver PRIVATE --warn-all)
target_link_libraries(jsonserver LINK_PUBLIC express)
install(TARGETS jsonserver RUNTIME DESTINATION bin)
set_target_properties(
    jsonserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET jsonserver PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path}
    )
endif(CHECK_INCLUDES)

add_executable(jsonclient jsonclient.cpp)
target_compile_options(jsonclient PRIVATE --warn-all)
target_link_libraries(jsonclient LINK_PUBLIC express)
install(TARGETS jsonclient RUNTIME DESTINATION bin)
set_target_properties(
    jsonclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET jsonclient PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path}
    )
endif(CHECK_INCLUDES)

add_executable(testparser testparsers.cpp)
target_compile_options(testparser PRIVATE --warn-all)
target_link_libraries(testparser LINK_PUBLIC express)
install(TARGETS testparser RUNTIME DESTINATION bin)
set_target_properties(
    testparser
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET testparser PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                   ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(client client.cpp)
target_compile_options(client PRIVATE --warn-all)
target_link_libraries(client LINK_PUBLIC http)
install(TARGETS client RUNTIME DESTINATION bin)
set_target_properties(
    client
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET client PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                               ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(client6 client6.cpp)
target_compile_options(client6 PRIVATE --warn-all)
target_link_libraries(client6 LINK_PUBLIC http)
install(TARGETS client6 RUNTIME DESTINATION bin)
set_target_properties(
    client6
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET client6 PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(client6clientcert client6clientcert.cpp)
target_compile_options(client6clientcert PRIVATE --warn-all)
target_link_libraries(client6clientcert LINK_PUBLIC http)
install(TARGETS client6clientcert RUNTIME DESTINATION bin)
set_target_properties(
    client6clientcert
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET client6clientcert PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                          ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(lowlevelclient lowlevelclient.cpp)
target_compile_options(lowlevelclient PRIVATE --warn-all)
target_link_libraries(lowlevelclient LINK_PUBLIC http)
install(TARGETS lowlevelclient RUNTIME DESTINATION bin)
set_target_properties(
    lowlevelclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET lowlevelclient PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                       ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(testregex testregex.cpp)
target_compile_options(testregex PRIVATE --warn-all)
target_link_libraries(testregex LINK_PUBLIC express)
install(TARGETS testregex RUNTIME DESTINATION bin)
set_target_properties(
    testregex
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET testregex PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                  ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)

add_executable(warema-jalousien warema-jalousien.cpp)
target_compile_options(warema-jalousien PRIVATE --warn-all)
target_link_libraries(warema-jalousien LINK_PUBLIC express)
install(TARGETS warema-jalousien RUNTIME DESTINATION bin)
set_target_properties(
    warema-jalousien
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET warema-jalousien PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                         ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)




add_executable(btlowlevelclient btlowlevelclient.cpp)
target_compile_options(btlowlevelclient PRIVATE --warn-all)
target_link_libraries(btlowlevelclient LINK_PUBLIC http)
install(TARGETS btlowlevelclient RUNTIME DESTINATION bin)
set_target_properties(
    btlowlevelclient
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
)
if(CHECK_INCLUDES)
    set_property(
        TARGET btlowlevelclient PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                       ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)




add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/.logger.conf"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/logger.conf"
            "${CMAKE_CURRENT_BINARY_DIR}/.logger.conf"
    DEPENDS "${CMAKE_SOURCE_DIR}/logger.conf"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/.logger.conf DESTINATION bin)

add_custom_target(
    copy_logger.conf ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/.logger.conf"
)
