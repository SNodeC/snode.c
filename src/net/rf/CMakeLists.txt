cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig REQUIRED)

pkg_check_modules(BLUEZ bluez)

if(BLUEZ_FOUND)

    add_library(
        net-rf SHARED SocketAddress.cpp stream/Socket.cpp
                      stream/ServerSocket.cpp stream/ClientSocket.cpp
    )
    add_library(snodec::net-rf ALIAS net-rf)

    target_include_directories(
        net-rf
        PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
               "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    target_link_libraries(
        net-rf PUBLIC snodec::core-legacy snodec::core-tls ${BLUEZ_LIBRARIES}
    )

    set_target_properties(
        net-rf PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-rf
    )

    install(
        TARGETS net-rf
        EXPORT snodecnet-rfTargets
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
        EXPORT snodecnet-rfTargets
        FILE snodecnet-rfTargets.cmake
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
