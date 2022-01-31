cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig REQUIRED)

pkg_check_modules(BLUEZ bluez)

if(BLUEZ_FOUND)
    set(NET-RF_CPP
        SocketAddress.cpp stream/Socket.cpp stream/ClientSocket.cpp
        stream/ConfigBind.cpp stream/ServerConfig.cpp stream/ServerSocket.cpp
    )

    set(NET-RF_H
        SocketAddress.h
        stream/Socket.h
        stream/ServerConfig.h
        stream/ServerSocket.h
        stream/ClientSocket.h
        stream/ConfigBind.h
        stream/legacy/ServerConfig.h
        stream/legacy/SocketServer.h
        stream/legacy/SocketClient.h
        stream/tls/ServerConfig.h
        stream/tls/SocketServer.h
        stream/tls/SocketClient.h
    )

    add_library(net-rf SHARED ${NET-RF_CPP} ${NET-RF_H})
    add_library(snodec::net-rf ALIAS net-rf)

    target_link_libraries(net-rf PUBLIC snodec::net ${BLUEZ_LIBRARIES})

    target_include_directories(
        net-rf
        PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    set_target_properties(
        net-rf PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-rf
    )

    install(
        TARGETS net-rf
        EXPORT snodec_net-rf_Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/rf
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
    )

    install(
        EXPORT snodec_net-rf_Targets
        FILE snodec_net-rf_Targets.cmake
        NAMESPACE snodec::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    )

else(BLUEZ_FOUND)
    message(
        WARNING
            " libbluetooth-dev not found:\n"
            "    If you want to get bluetooth support you should install libbluetooth-dev."
    )
endif(BLUEZ_FOUND)
