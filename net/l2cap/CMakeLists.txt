cmake_minimum_required(VERSION 3.0)


find_package(PkgConfig REQUIRED)

pkg_check_modules(BLUEZ bluez)

if(BLUEZ_FOUND)

add_library(
    net-l2cap SHARED L2CapAddress.cpp stream/Socket.cpp
)
add_library(snodec::net-l2cap ALIAS net-l2cap)

target_include_directories(
    net-l2cap
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net-l2cap PUBLIC snodec::core-legacy snodec::core-tls ${BLUEZ_LIBRARIES})

set_target_properties(net-l2cap PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-l2cap)

install(
    TARGETS net-l2cap
    EXPORT snodecnet-l2capTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/l2cap
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnet-l2capTargets
    FILE snodecnet-l2capTargets.cmake
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
