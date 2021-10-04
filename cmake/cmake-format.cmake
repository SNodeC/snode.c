# Additional targets to perform cmake-format

find_program(CMAKE_FORMAT "cmake-format")

if(CMAKE_FORMAT)
    file(GLOB_RECURSE CMAKELISTS_TXT_FILES ${CMAKE_SOURCE_DIR}/CMakeLists.txt
         ${CMAKE_SOURCE_DIR}/cmake/*.cmake ${CMAKE_SOURCE_DIR}/*.cmake.in
    )
    add_custom_command(
        OUTPUT format-cmds
        APPEND
        COMMAND ${CMAKE_FORMAT} -i ${CMAKELISTS_TXT_FILES}
        COMMENT "Auto formatting of all CMakeLists.txt files"
    )
else(CMAKE_FORMAT)
    message(WARNING "cmake-format is neccesarry for consistent cmake files format")
endif(CMAKE_FORMAT)
