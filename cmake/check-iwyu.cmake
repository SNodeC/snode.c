find_program(iwyu_path NAMES include-what-you-use iwyu)
if(iwyu_path)
    option(CHECK_INCLUDES "Check used headers")

    if(CHECK_INCLUDES)
        set(iwyu_path_and_options
            ${iwyu_path}
            -Xiwyu
            --verbose=1
            -Xiwyu
            --cxx17ns
            -Xiwyu
            --quoted_includes_first
            -Xiwyu
            --check_also='${PROJECT_SOURCE_DIR}/*'
        )

        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
    endif(CHECK_INCLUDES)
else()
    message(FATAL_ERROR "Could not find the program include-what-you-use")
endif()
