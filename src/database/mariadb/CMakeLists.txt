cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig REQUIRED)

pkg_check_modules(MARIADB mariadb)

if(MARIADB_FOUND)

    set(MARIADB_CPP
        MariaDBClient.cpp
        MariaDBClientAPI.cpp
        MariaDBCommand.cpp
        MariaDBCommandBlocking.cpp
        MariaDBCommandNonBlocking.cpp
        MariaDBCommandSequence.cpp
        MariaDBConnection.cpp
        commands/sync/MariaDBAffectedRowsCommand.cpp
        commands/sync/MariaDBUseResultCommand.cpp
        commands/async/MariaDBAutoCommitCommand.cpp
        commands/async/MariaDBCommitCommand.cpp
        commands/async/MariaDBConnectCommand.cpp
        commands/async/MariaDBExecCommand.cpp
        commands/async/MariaDBFetchRowCommand.cpp
        commands/async/MariaDBFreeResultCommand.cpp
        commands/async/MariaDBQueryCommand.cpp
        commands/async/MariaDBRollbackCommand.cpp
    )

    set(MARIADB_H
        MariaDBClient.h
        MariaDBClientAPI.h
        MariaDBCommand.h
        MariaDBCommandBlocking.h
        MariaDBCommandNonBlocking.h
        MariaDBCommandSequence.h
        MariaDBConnection.h
        MariaDBConnectionDetails.h
        commands/sync/MariaDBAffectedRowsCommand.h
        commands/sync/MariaDBUseResultCommand.h
        commands/async/MariaDBAutoCommitCommand.h
        commands/async/MariaDBCommitCommand.h
        commands/async/MariaDBConnectCommand.h
        commands/async/MariaDBExecCommand.h
        commands/async/MariaDBFetchRowCommand.h
        commands/async/MariaDBFreeResultCommand.h
        commands/async/MariaDBQueryCommand.h
        commands/async/MariaDBRollbackCommand.h
    )

    add_library(db-mariadb SHARED ${MARIADB_CPP} ${MARIADB_H})
    add_library(snodec::db-mariadb ALIAS db-mariadb)
    target_link_libraries(db-mariadb PUBLIC snodec::core ${MARIADB_LIBRARIES})
    target_include_directories(
        db-mariadb
        PUBLIC ${MARIADB_INCLUDE_DIRS}
               "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
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
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/snode.c/database/mariadb
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
    )

endif(MARIADB_FOUND)
