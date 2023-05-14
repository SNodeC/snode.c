cmake_minimum_required(VERSION 3.18)

find_package(PkgConfig REQUIRED)

pkg_check_modules(BLUEZ bluez)

if(BLUEZ_FOUND)
    set(NET-L2_CPP SocketAddress.cpp PhysicalSocket.cpp
                   config/ConfigAddress.cpp
    )

    set(NET-L2_H PhysicalSocket.hpp PhysicalSocket.h SocketAddress.h
                 config/ConfigAddress.h
    )

    add_library(net-l2 SHARED ${NET-L2_CPP} ${NET-L2_H})
    add_library(snodec::net-l2 ALIAS net-l2)

    target_link_libraries(net-l2 PUBLIC snodec::net ${BLUEZ_LIBRARIES})

    target_include_directories(
        net-l2
        PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    set_target_properties(
        net-l2 PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-l2
    )

    install(
        TARGETS net-l2
        EXPORT snodec_net-l2_Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-l2
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/l2
        COMPONENT net-l2
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
        PATTERN "stream" EXCLUDE
        PATTERN "dgram" EXCLUDE
    )

    install(
        EXPORT snodec_net-l2_Targets
        FILE snodec_net-l2_Targets.cmake
        NAMESPACE snodec::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
        COMPONENT net-l2
    )

    add_subdirectory(stream)

else(BLUEZ_FOUND)
    message(
        WARNING
            " libbluetooth-dev not found:\n"
            "    If you want to get bluetooth support you should install libbluetooth-dev."
    )
endif(BLUEZ_FOUND)
