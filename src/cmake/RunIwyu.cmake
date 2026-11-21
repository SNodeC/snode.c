# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
# Retain GCC code-generation flags for compilation, not Clang-based analysis.
math(EXPR last_argument "${CMAKE_ARGC} - 1")
set(iwyu_arguments)
foreach(index RANGE 4 ${last_argument})
    if(NOT CMAKE_ARGV${index} STREQUAL "-fno-gnu-unique")
        string(REPLACE ";" "\\;" argument "${CMAKE_ARGV${index}}")
        list(APPEND iwyu_arguments "${argument}")
    endif()
endforeach()
execute_process(COMMAND ${iwyu_arguments} RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "include-what-you-use exited with ${result}")
endif()
