# Additional targets to perform clang-format

# Get all project files
if(NOT CHECK_CXX_SOURCE_FILES)
  message(
    FATAL_ERROR
      "Variable CHECK_CXX_SOURCE_FILES not defined - set it to the list of files to auto-format"
  )
  return()
endif()

# Adding clang-format check and formatter if found
find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
  add_custom_command(
    OUTPUT format-cmds
    APPEND
    COMMAND ${CLANG_FORMAT} -i ${CHECK_CXX_SOURCE_FILES}
    COMMENT "Auto formatting of all source files")

  # ADD_CUSTOM_TARGET( check-format COMMAND ${CLANG_FORMAT}
  # -output-replacements-xml ${CHECK_CXX_SOURCE_FILES} # print output | tee
  # ${CMAKE_BINARY_DIR}/check_format_file.txt | grep -c "replacement " | tr -d
  # "[:cntrl:]" && echo " replacements necessary" # WARNING: fix to stop with
  # error if there are problems COMMAND ! grep -c "replacement "
  # ${CMAKE_BINARY_DIR}/check_format_file.txt > /dev/null COMMENT "Checking
  # format compliance" )
endif()
