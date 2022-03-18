cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig REQUIRED)

pkg_check_modules(MARIADB mariadb)

set(MARIADB_CPP
    MariaDBClient.cpp MariaDBCommand.cpp MariaDBConnection.cpp
    commands/MariaDBConnectCommand.cpp commands/MariaDBQueryCommand.cpp
)

set(MARIADB_H
    MariaDBClient.h MariaDBCommand.h MariaDBConnection.h
    MariaDBConnectionDetails.h commands/MariaDBConnectCommand.h
    commands/MariaDBQueryCommand.h
)

add_library(db-mariadb SHARED ${MARIADB_CPP} ${MARIADB_H})
add_library(snodec::db-mariadb ALIAS db-mariadb)
target_link_libraries(db-mariadb PUBLIC snodec::core ${MARIADB_LIBRARIES})
target_include_directories(
    db-mariadb
    PUBLIC ${MARIADB_INCLUDE_DIRS} "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)
set_target_properties(
    db-mariadb PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-db-mariadb
)
install(
    TARGETS db-mariadb
    EXPORT snodec_db-mariadb_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)
install(
    EXPORT snodec_db-mariadb_Targets
    FILE snodec_db-mariadb_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_executable(testmariadb testmariadb.cpp)
target_link_libraries(testmariadb db-mariadb)
target_include_directories(
    testmariadb
    PUBLIC ${MARIADB_INCLUDE_DIRS} "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)
install(TARGETS testmariadb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
