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
    message(WARNING " cmake-format not found:\n"
                    "    cmake-format is used to format all CMakeLists.txt files consitently.\n"
                    "    It is highly recommented to install it, if you intend to modify the code of SNode.C.\n"
                    "    In case you have it installed run \"cmake --target format\" to format all CMakeLists.txt files.\n"
                    "    If you do not want to contribute to SNode.C, you can ignore this warning.\n")
endif(CMAKE_FORMAT)
