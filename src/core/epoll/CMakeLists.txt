cmake_minimum_required(VERSION 3.0)

set(MUX_EPOLL_CPP DescriptorEventDispatcher.cpp EventDispatcher.cpp)

set(MUX_EPOLL_H DescriptorEventDispatcher.h EventDispatcher.h)

add_library(mux-epoll SHARED ${MUX_EPOLL_CPP} ${MUX_EPOLL_H})
add_library(snodec::mux-epoll ALIAS mux-epoll)

target_include_directories(
    mux-epoll
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mux-epoll PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-mux-epoll
)

install(
    TARGETS mux-epoll
    EXPORT snodec_mux-epoll_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR}
)

install(
    EXPORT snodec_mux-epoll_Targets
    FILE snodec_mux-epoll_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
