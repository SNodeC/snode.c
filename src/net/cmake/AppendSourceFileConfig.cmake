function(append_source_file_config source_file CONFIG_OPTION description default_value)
    if ("${${CONFIG_OPTION}}" STREQUAL "")
        set(value ${default_value})
    elseif("${ARGN}" STREQUAL "")
        set(value ${${CONFIG_OPTION}})
    else()
        set(value ${ARGN})
    endif()

    set(SNODEC_${CONFIG_OPTION}
        ${value}
        CACHE STRING "${description}"
    )

    set_property(
      SOURCE ${source_file}
      APPEND PROPERTY COMPILE_DEFINITIONS ${CONFIG_OPTION}=${SNODEC_${CONFIG_OPTION}}
    )

    message(STATUS "Appending compile definition to '${source_file}': " '${CONFIG_OPTION}=${SNODEC_${CONFIG_OPTION}}')
endfunction(append_source_file_config source_file CONFIG_OPTION description default_value)
