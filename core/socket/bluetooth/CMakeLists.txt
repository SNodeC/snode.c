cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig REQUIRED)

pkg_check_modules(BLUEZ bluez)

if(BLUEZ_FOUND)

    add_library(
        net-bt SHARED address/L2CapAddress.cpp address/RfCommAddress.cpp
                      l2cap/Socket.cpp rfcomm/Socket.cpp
    )
    add_library(snodec::net-bt ALIAS net-bt)

    target_include_directories(
        net-bt
        PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
               "$<BUILD_INTERFACE:${BLUEZ_INCLUDE_DIRS}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    target_link_libraries(
        net-bt PUBLIC snodec::net-legacy snodec::net-tls ${BLUEZ_LIBRARIES}
    )

    set_target_properties(
        net-bt PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-bt
    )

    install(
        TARGETS net-bt
        EXPORT snodecnet-btTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket/bluetooth
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
    )

    install(
        EXPORT snodecnet-btTargets
        FILE snodecnet-btTargets.cmake
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
