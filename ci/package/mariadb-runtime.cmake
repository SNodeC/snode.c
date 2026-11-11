if(SNODEC_MARIADB_RUNTIME)
    include("${SNODEC_MARIADB_RUNTIME}/metadata.cmake")
    install(
        FILES "${SNODEC_MARIADB_RUNTIME}/usr/lib64/libmariadb.so.3"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT libmariadb
    )
    install(
        DIRECTORY "${SNODEC_MARIADB_RUNTIME}/usr/lib64/mariadb/"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/mariadb"
        COMPONENT libmariadb
        USE_SOURCE_PERMISSIONS
    )
    install(
        FILES "${SNODEC_MARIADB_RUNTIME}/COPYING.LIB"
              "${SNODEC_MARIADB_RUNTIME}/README"
        DESTINATION "share/licenses/snodec-libmariadb"
        COMPONENT libmariadb
    )
    set(CPACK_RPM_LIBMARIADB_PACKAGE_PROVIDES
        "mariadb-connector-c = ${SNODEC_MARIADB_BASE}, mariadb-connector-c%{?_isa} = ${SNODEC_MARIADB_BASE}"
    )
    set(CPACK_RPM_LIBMARIADB_PACKAGE_OBSOLETES
        "mariadb-connector-c <= ${SNODEC_MARIADB_BASE}"
    )
    set(CPACK_RPM_LIBMARIADB_PACKAGE_CONFLICTS "mariadb-connector-c")
    set(CPACK_RPM_LIBMARIADB_PACKAGE_REQUIRES
        "mariadb-connector-c-config = ${SNODEC_MARIADB_BASE}"
    )
    set(CPACK_RPM_LIBMARIADB_PACKAGE_LICENSE
        "LGPL-2.1-or-later AND PHP-3.0 AND PHP-3.01 AND LicenseRef-Fedora-Public-Domain"
    )
    set(CPACK_RPM_DB-MARIADB_PACKAGE_REQUIRES
        "snodec-libmariadb = ${SNODEC_MARIADB_VERSION}"
    )
endif()
