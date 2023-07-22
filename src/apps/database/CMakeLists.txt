cmake_minimum_required(VERSION 3.18)

if(LIBMARIADB_FOUND)

    add_executable(testmariadb testmariadb.cpp)
    target_link_libraries(testmariadb PRIVATE snodec::db-mariadb)
    target_include_directories(
        testmariadb
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${MARIADB_INCLUDE_DIRS}
               "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )
    install(TARGETS testmariadb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                        COMPONENT apps
    )

endif(LIBMARIADB_FOUND)
