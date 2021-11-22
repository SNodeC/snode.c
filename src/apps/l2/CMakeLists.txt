cmake_minimum_required(VERSION 3.0)

get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR} NAME)

set(NET ${CURRENT_DIR})

add_compile_definitions(NET=${NET})

if(BLUEZ_FOUND)

endif(BLUEZ_FOUND)
