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

    add_executable(
        test_mysql_real_connect_start test_mysql_real_connect_start.cpp
    )
    target_link_libraries(
        test_mysql_real_connect_start PRIVATE ${LIBMARIADB_LIBRARIES}
    )
    target_include_directories(
        test_mysql_real_connect_start
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${LIBMARIADB_INCLUDE_DIRS}
               "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    install(TARGETS test_mysql_real_connect_start
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
    )

endif(LIBMARIADB_FOUND)
