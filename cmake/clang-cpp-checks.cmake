# Additional targets to perform clang-format

# Get all project files if(NOT CHECK_CXX_SOURCE_FILES) message( FATAL_ERROR
# "Variable CHECK_CXX_SOURCE_FILES not defined - set it to the list of files to
# auto-format" ) return() endif()

# Adding clang-format check and formatter if found
find_program(CLANG_FORMAT "clang-format")

if(CLANG_FORMAT)
    # Set the source files to clang - format
    file(GLOB_RECURSE CHECK_CXX_SOURCE_FILES ${CMAKE_SOURCE_DIR}/*.[tch]pp
         ${CMAKE_SOURCE_DIR}/*.h
    )

    # Remove strings matching given regular expression from a list.
    # @param(in,out) aItems Reference of a list variable to filter. @param
    # aRegEx Value of regular expression to match.
    function(filter_items aItems aRegEx)
        # For each item in our list
        foreach(item ${${aItems}})
            # Check if our items matches our regular expression
            if("${item}" MATCHES ${aRegEx})
                # Remove current item from our list
                list(REMOVE_ITEM ${aItems} ${item})
            endif("${item}" MATCHES ${aRegEx})
        endforeach(item)
        # Provide output parameter
        set(${aItems}
            ${${aItems}}
            PARENT_SCOPE
        )
    endfunction(filter_items)

    filter_items(CHECK_CXX_SOURCE_FILES "/build/")

    add_custom_command(
        OUTPUT format-cmds
        APPEND
        COMMAND ${CLANG_FORMAT} -i ${CHECK_CXX_SOURCE_FILES}
        COMMENT "Auto formatting of all source files"
    )

    # ADD_CUSTOM_TARGET( check-format COMMAND ${CLANG_FORMAT}
    # -output-replacements-xml ${CHECK_CXX_SOURCE_FILES} # print output | tee
    # ${CMAKE_BINARY_DIR}/check_format_file.txt | grep -c "replacement " | tr -d
    # "[:cntrl:]" && echo " replacements necessary" # WARNING: fix to stop with
    # error if there are problems COMMAND ! grep -c "replacement "
    # ${CMAKE_BINARY_DIR}/check_format_file.txt > /dev/null COMMENT "Checking
    # format compliance" )
else(CLANG_FORMAT)
    message(
        WARNING
            " clang-format not found:\n"
            "    clang-format is used to format all source files consistently.\n"
            "    It is highly recommented to install it, if you intend to modify the code of SNode.C.\n"
            "    In case you have it installed run \"cmake --target format\" to format all source files.\n"
            "    If you do not want to contribute to SNode.C, you can ignore this warning.\n"
    )
endif(CLANG_FORMAT)
