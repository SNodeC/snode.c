find_package(PkgConfig REQUIRED)
pkg_check_modules(NLOHMANN_JSON QUIET nlohmann_json)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    nlohmann_json
    REQUIRED_VARS NLOHMANN_JSON_FOUND
    VERSION_VAR NLOHMANN_JSON_VERSION
)
if(nlohmann_json_FOUND AND NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED GLOBAL)
    set_target_properties(
        nlohmann_json::nlohmann_json PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                "${NLOHMANN_JSON_INCLUDE_DIRS}"
    )
    add_library(nlohmann_json ALIAS nlohmann_json::nlohmann_json)
endif()
