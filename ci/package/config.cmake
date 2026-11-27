# Evaluated by CPack after release/version overrides have been applied.
set(names "${CPACK_PACKAGE_NAME}\n")
set(full_requires "")
foreach(component IN LISTS CPACK_COMPONENTS_ALL)
    if(NOT component STREQUAL "full")
        string(TOLOWER "${CPACK_PACKAGE_NAME}-${component}" name)
        string(APPEND names "${name}\n")
        list(APPEND full_requires
             "${name} = ${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}"
        )
    endif()
endforeach()
list(JOIN full_requires ", " CPACK_RPM_FULL_PACKAGE_REQUIRES)
# Publication inventory comes from the same component model as the packages.
file(MAKE_DIRECTORY "${CPACK_OUTPUT_FILE_PREFIX}")
file(WRITE "${CPACK_OUTPUT_FILE_PREFIX}/${CPACK_PACKAGE_NAME}.packages"
     "${names}"
)

set(CPACK_DEBIAN_COMMON_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_LIST_DIR}/debian/postinst"
)
set(CPACK_RPM_COMMON_POST_INSTALL_SCRIPT_FILE
    "${CMAKE_CURRENT_LIST_DIR}/rpm/postinst"
)
if(DEFINED ENV{DEPENDENCIES_JSON})
    string(JSON count LENGTH "$ENV{DEPENDENCIES_JSON}")
    if(NOT count EQUAL 0)
        message(FATAL_ERROR "SNode.C has no resolved package dependencies")
    endif()
endif()
