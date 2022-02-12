cmake_minimum_required(VERSION 3.0)

set(MUX_SELECT_CPP DescriptorEventPublisher.cpp EventMultiplexer.cpp)

set(MUX_SELECT_H DescriptorEventPublisher.h EventMultiplexer.h)

add_library(mux-select SHARED ${MUX_SELECT_CPP} ${MUX_SELECT_H})
add_library(snodec::mux-select ALIAS mux-select)

target_include_directories(
    mux-select
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mux-select PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-mux-select
)

install(
    TARGETS mux-select
    EXPORT snodec_mux-select_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR}
)

install(
    EXPORT snodec_mux-select_Targets
    FILE snodec_mux-select_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
