cmake_minimum_required(VERSION 3.0)

set(NET_CPP
    config/ConfigBase.cpp config/ConfigBacklog.cpp config/ConfigConnection.cpp
    config/ConfigLegacy.cpp config/ConfigTls.cpp
)

set(NET_H
    config/ConfigAddressLocal.h
    config/ConfigAddressLocal.hpp
    config/ConfigAddressRemote.h
    config/ConfigAddressRemote.hpp
    config/ConfigBase.h
    config/ConfigBacklog.h
    config/ConfigConnection.h
    config/ConfigLegacy.h
    config/ConfigTls.h
)

add_library(net SHARED ${NET_CPP} ${NET_H})
add_library(snodec::net ALIAS net)

target_include_directories(
    net
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(net PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net)

install(
    TARGETS net
    EXPORT snodec_net_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "in" EXCLUDE
    PATTERN "in6" EXCLUDE
    PATTERN "l2" EXCLUDE
    PATTERN "rf" EXCLUDE
    PATTERN "un" EXCLUDE
)

install(
    EXPORT snodec_net_Targets
    FILE snodec_net_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(in)
add_subdirectory(in6)
add_subdirectory(l2)
add_subdirectory(rf)
add_subdirectory(un)
