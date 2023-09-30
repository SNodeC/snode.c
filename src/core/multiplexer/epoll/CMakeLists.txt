cmake_minimum_required(VERSION 3.18)

set(MUX_EPOLL_CPP DescriptorEventPublisher.cpp EventMultiplexer.cpp)

set(MUX_EPOLL_H DescriptorEventPublisher.h EventMultiplexer.h)

add_library(mux-epoll SHARED ${MUX_EPOLL_CPP} ${MUX_EPOLL_H})
add_library(snodec::mux-epoll ALIAS mux-epoll)

target_include_directories(
    mux-epoll
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mux-epoll PROPERTIES SOVERSION ${SNODEC_SO_VERSION} OUTPUT_NAME
                                                       snodec-core-mux-epoll
)

install(
    TARGETS mux-epoll
    EXPORT snodec_mux-epoll_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR} COMPONENT mux-epoll
)

install(
    EXPORT snodec_mux-epoll_Targets
    FILE snodec_mux-epoll_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mux-epoll
)
