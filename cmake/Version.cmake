# Release tags are authoritative in a Git checkout; archives use VERSION.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../VERSION")
file(READ "${CMAKE_CURRENT_LIST_DIR}/../VERSION" RELEASE_VERSION)
string(STRIP "${RELEASE_VERSION}" RELEASE_VERSION)
if(NOT RELEASE_VERSION MATCHES "^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$")
    message(FATAL_ERROR "VERSION must contain MAJOR.MINOR.PATCH")
endif()
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../.git")
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(COMMAND "${GIT_EXECUTABLE}" tag --merged HEAD --sort=-version:refname
                        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
                        OUTPUT_VARIABLE release_tags OUTPUT_STRIP_TRAILING_WHITESPACE
                        RESULT_VARIABLE version_result)
        if(NOT version_result EQUAL 0)
            message(FATAL_ERROR "Cannot inspect release tags")
        endif()
        string(REPLACE "\n" ";" release_tags "${release_tags}")
        # Fewest intervening commits wins; highest semantic version breaks ties.
        set(nearest_distance "")
        foreach(tag IN LISTS release_tags)
            if(tag MATCHES "^v(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$")
                execute_process(COMMAND "${GIT_EXECUTABLE}" rev-list --count "${tag}..HEAD"
                                WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
                                OUTPUT_VARIABLE distance OUTPUT_STRIP_TRAILING_WHITESPACE
                                RESULT_VARIABLE version_result)
                if(NOT version_result EQUAL 0)
                    message(FATAL_ERROR "Cannot determine release tag distance")
                endif()
                if(nearest_distance STREQUAL "" OR distance LESS nearest_distance)
                    set(nearest_distance "${distance}")
                    string(SUBSTRING "${tag}" 1 -1 RELEASE_VERSION)
                endif()
            endif()
        endforeach()
    endif()
endif()
