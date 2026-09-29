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
        # Track both ref contents and additions/removals, including packed refs.
        # Git resolves shared refs correctly for linked worktrees.
        execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse
                        --git-path HEAD --git-path "refs/heads/*" --git-path "refs/tags/*"
                        --git-path packed-refs --git-path shallow
                        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
                        OUTPUT_VARIABLE git_ref_paths OUTPUT_STRIP_TRAILING_WHITESPACE
                        RESULT_VARIABLE version_result)
        if(NOT version_result EQUAL 0)
            message(FATAL_ERROR "Cannot locate Git version inputs")
        endif()
        string(REPLACE "\n" ";" git_ref_paths "${git_ref_paths}")
        set(git_ref_patterns "")
        foreach(path IN LISTS git_ref_paths)
            get_filename_component(path "${path}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
            list(APPEND git_ref_patterns "${path}")
        endforeach()
        file(GLOB_RECURSE git_ref_files LIST_DIRECTORIES FALSE CONFIGURE_DEPENDS ${git_ref_patterns})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${git_ref_files})
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
