cmake_minimum_required(VERSION 3.0)

get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR} NAME)

set(NET ${CURRENT_DIR})
set(TYPE legacy)

add_executable(${NET}web${TYPE}client webclient.cpp)
target_link_libraries(${NET}web${TYPE}client PRIVATE snodec::httpclient)
target_compile_definitions(${NET}webclient PRIVATE TYPE=${TYPE})
install(TARGETS ${NET}webclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
