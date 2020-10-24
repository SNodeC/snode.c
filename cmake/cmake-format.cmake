# Additional targets to perform cmake-format

find_program(CMAKE_FORMAT "cmake-format")
if(CMAKE_FORMAT)
  file(GLOB_RECURSE CMAKELISTS_TXT_FILES CMakeLists.txt cmake/*.cmake)
  add_custom_target(
    format-cmake
    COMMAND ${CMAKE_FORMAT} -i ${CMAKELISTS_TXT_FILES}
    COMMENT "Auto formatting of all CMakeLists.txt files")
endif(CMAKE_FORMAT)
